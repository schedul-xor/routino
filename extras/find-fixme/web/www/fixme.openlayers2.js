//
// Routino (extras) fixme web page Javascript
//
// Part of the Routino routing software.
//
// This file Copyright 2008-2014, 2019, 2020 Andrew M. Bishop
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Initialisation /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Process the URL query string and extract the arguments

var legal={"^lon"  : "^[-0-9.]+$",
           "^lat"  : "^[-0-9.]+$",
           "^zoom" : "^[-0-9.]+$"};

var args={};

if(location.search.length>1)
  {
   var query,queries;

   query=location.search.replace(/^\?/,"");
   query=query.replace(/;/g,"&");
   queries=query.split("&");

   for(var i=0;i<queries.length;i++)
     {
      queries[i].match(/^([^=]+)(=(.*))?$/);

      var k=RegExp.$1;
      var v=decodeURIComponent(RegExp.$3);

      for(var l in legal)
        {
         if(k.match(RegExp(l)) && v.match(RegExp(legal[l])))
            args[k]=v;
        }
     }
  }


////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Map handling /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

var map;
var layerMap=[], layerHighlights, layerVectors;
var vectorData=[];
var epsg4326, epsg900913;
var select;

//
// Initialise the 'map' object
//

function map_init()             // called from fixme.html
{
 // Create the map (Map URLs and limits are in mapprops.js)

 epsg4326=new OpenLayers.Projection("EPSG:4326");
 epsg900913=new OpenLayers.Projection("EPSG:900913");

 map = new OpenLayers.Map ("map",
                           {
                            controls:[
                                      new OpenLayers.Control.Navigation(),
                                      new OpenLayers.Control.PanZoomBar(),
                                      new OpenLayers.Control.ScaleLine(),
                                      new OpenLayers.Control.LayerSwitcher()
                                      ],

                            projection: epsg900913,
                            displayProjection: epsg4326,

                            minZoomLevel: mapprops.zoomout,
                            numZoomLevels: mapprops.zoomin-mapprops.zoomout+1,
                            maxResolution: 156543.03390625 / Math.pow(2,mapprops.zoomout),

                            restrictedExtent: new OpenLayers.Bounds(mapprops.westedge,mapprops.southedge,mapprops.eastedge,mapprops.northedge).transform(epsg4326,epsg900913)
                           });

 // Get a URL for the tile (mostly copied from OpenLayers/Layer/XYZ.js).

 function limitedUrl(bounds)
 {
  var res = this.map.getResolution();

  var x = Math.round((bounds.left - this.maxExtent.left) / (res * this.tileSize.w));
  var y = Math.round((this.maxExtent.top - bounds.top) / (res * this.tileSize.h));
  var z = this.map.getZoom() + this.map.minZoomLevel;

  var limit = Math.pow(2, z);
  x = ((x % limit) + limit) % limit;

  var xyz = {"x": x, "y": y, "z": z};
  var url = this.url;

  if (OpenLayers.Util.isArray(url))
    {
     var s = "" + xyz.x + xyz.y + xyz.z;
     url = this.selectUrl(s, url);
    }

  return OpenLayers.String.format(url, xyz);
 }

 // Add map tile layers

 for(var l=0; l<mapprops.mapdata.length; l++)
   {
    var urls;

    if(OpenLayers.Util.isArray(mapprops.mapdata[l].tiles.subdomains))
      {
       urls=[];

       for(var s=0; s<mapprops.mapdata[l].tiles.subdomains.length; s++)
          urls.push(mapprops.mapdata[l].tiles.url.replace(/\${s}/,mapprops.mapdata[l].tiles.subdomains[s]));
      }
    else
       urls=mapprops.mapdata[l].tiles.url;

    layerMap[l] = new OpenLayers.Layer.TMS(mapprops.mapdata[l].label,
                                           urls,
                                           {
                                            getURL: limitedUrl,
                                            displayOutsideMaxExtent: true,
                                            buffer: 1
                                           });
    map.addLayer(layerMap[l]);
   }

 // Update the attribution if the layer changes

 function change_attribution_event(event)
 {
  for(var l=0; l<mapprops.mapdata.length; l++)
     if(layerMap[l] == event.layer)
        change_attribution(l);
 }

 map.events.register("changelayer",layerMap,change_attribution_event);

 function change_attribution(l)
 {
  var data_url =mapprops.mapdata[l].attribution.data_url;
  var data_text=mapprops.mapdata[l].attribution.data_text;
  var tile_url =mapprops.mapdata[l].attribution.tile_url;
  var tile_text=mapprops.mapdata[l].attribution.tile_text;

  document.getElementById("attribution_data").innerHTML="<a href=\"" + data_url + "\" target=\"data_attribution\">" + data_text + "</a>";
  document.getElementById("attribution_tile").innerHTML="<a href=\"" + tile_url + "\" target=\"tile_attribution\">" + tile_text + "</a>";
 }

 change_attribution(0);

 // Add two vectors layers (one for highlights that display behind the vectors)

 layerHighlights = new OpenLayers.Layer.Vector("Highlights",{displayInLayerSwitcher: false});
 map.addLayer(layerHighlights);

 layerVectors = new OpenLayers.Layer.Vector("Markers",{displayInLayerSwitcher: false});
 map.addLayer(layerVectors);

 // Handle feature selection and popup

 select = new OpenLayers.Control.SelectFeature(layerVectors,
                                               {onSelect: selectFeature, onUnselect: unselectFeature});

 map.addControl(select);
 select.activate();

 createPopup();

 // Move the map

 map.events.register("moveend", map, (function() { displayMoreData();}));

 var lon =args["lon"];
 var lat =args["lat"];
 var zoom=args["zoom"];

 if(lon !== undefined && lat !== undefined && zoom !== undefined)
   {
    lat  = Number(lat);
    lon  = Number(lon);
    zoom = Number.parseInt(Number(zoom)+0.5);

    if(lon<mapprops.westedge) lon=mapprops.westedge;
    if(lon>mapprops.eastedge) lon=mapprops.eastedge;

    if(lat<mapprops.southedge) lat=mapprops.southedge;
    if(lat>mapprops.northedge) lat=mapprops.northedge;

    if(zoom<mapprops.zoomout) zoom=mapprops.zoomout;
    if(zoom>mapprops.zoomin)  zoom=mapprops.zoomin;

    var lonlat = new OpenLayers.LonLat(lon,lat);
    lonlat.transform(epsg4326,epsg900913);

    map.moveTo(lonlat,zoom-map.minZoomLevel);
   }
 else
   {
    map.setCenter(map.restrictedExtent.getCenterLonLat(), map.getZoomForExtent(map.restrictedExtent,false));
    map.maxResolution = map.getResolution();
   }

 // Unhide editing URL if variable set

 if(mapprops.editurl !== undefined && mapprops.editurl !== "")
   {
    var edit_url=document.getElementById("edit_url");

    edit_url.style.display="";
    edit_url.href=mapprops.editurl;
   }

 updateURLs(false);
}


//
// Format a number in printf("%.5f") format.
//

function format5f(number)
{
 var newnumber=Math.floor(number*100000+0.5);
 var delta=0;

 if(newnumber>=0 && newnumber<100000) delta= 100000;
 if(newnumber<0 && newnumber>-100000) delta=-100000;

 var string=String(newnumber+delta);

 var intpart =string.substring(0,string.length-5);
 var fracpart=string.substring(string.length-5,string.length);

 if(delta>0) intpart="0";
 if(delta<0) intpart="-0";

 return(intpart + "." + fracpart);
}


//
// Build a set of URL arguments for the map location
//

function buildMapArguments()
{
 var lonlat = map.getCenter().clone();
 lonlat.transform(epsg900913,epsg4326);

 var zoom = map.getZoom() + map.minZoomLevel;

 return "lat=" + format5f(lonlat.lat) + ";lon=" + format5f(lonlat.lon) + ";zoom=" + zoom;
}


//
// Update the URLs
//

function updateURLs(addhistory)
{
 var mapargs=buildMapArguments();
 var libargs=";library=" + mapprops.library;

 if(!mapprops.libraries)
    libargs="";

 var links=document.getElementsByTagName("a");

 for(var i=0; i<links.length; i++)
   {
    var element=links[i];

    if(element.id == "permalink_url")
       element.href=location.pathname + "?" + mapargs + libargs;

    if(element.id == "edit_url")
       element.href=mapprops.editurl + "?" + mapargs;
   }

 if(addhistory)
    history.replaceState(null, null, location.pathname + "?" + mapargs + libargs);
}


////////////////////////////////////////////////////////////////////////////////
///////////////////////// Popup and selection handling /////////////////////////
////////////////////////////////////////////////////////////////////////////////

var popup=null;

//
// Create a popup - independent of map because want it fixed on screen not fixed on map.
//

function createPopup()
{
 popup=document.createElement("div");

 popup.className = "popup";

 popup.innerHTML = "<span></span>";

 popup.style.display = "none";

 popup.style.position = "fixed";
 popup.style.top = "-4000px";
 popup.style.left = "-4000px";
 popup.style.zIndex = "100";

 popup.style.padding = "5px";

 popup.style.opacity=0.85;
 popup.style.backgroundColor="#C0C0C0";
 popup.style.border="4px solid #404040";

 document.body.appendChild(popup);
}


//
// Draw a popup - independent of map because want it fixed on screen not fixed on map.
//

function drawPopup(html)
{
 if(html===null)
   {
    popup.style.display="none";
    return;
   }

 if(popup.style.display=="none")
   {
    var map_div=document.getElementById("map");

    popup.style.left  =map_div.offsetParent.offsetLeft+map_div.offsetLeft+60 + "px";
    popup.style.top   =                                map_div.offsetTop +30 + "px";
    popup.style.width =map_div.clientWidth-120 + "px";

    popup.style.display="";
   }

 var close="<span style='float: right; cursor: pointer;' onclick='drawPopup(null)'>X</span>";

 popup.innerHTML=close+html;
}


//
// Select a feature
//

function selectFeature(feature)
{
 if(feature.attributes.dump)
    ajaxGET("fixme.cgi?dump=" + feature.attributes.dump, runDumpSuccess);

 layerHighlights.destroyFeatures();

 var highlight_style = new OpenLayers.Style({},{strokeColor: "#F0F000",strokeWidth: 8,
                                                fillColor: "#F0F000",pointRadius: 4});

 var highlight = new OpenLayers.Feature.Vector(feature.geometry.clone(),{},highlight_style);

 layerHighlights.addFeatures([highlight]);
}


//
// Un-select a feature
//

function unselectFeature(feature)
{
 layerHighlights.destroyFeatures();

 drawPopup(null);
}


//
// Display the dump data
//

function runDumpSuccess(response)
{
 var string=response.responseText;

 if(mapprops.browseurl !== undefined && mapprops.browseurl !== "")
   {
    var types=["node", "way", "relation"];

    for(var t in types)
      {
       var type=types[t];

       var regexp=RegExp(type + " id=&#39;([0-9]+)&#39;");

       var match=string.match(regexp);

       if(match !== null)
         {
          var id=match[1];

          string=string.replace(regexp,type + " id=&#39;<a href='" + mapprops.browseurl + "/" + type + "/" + id + "' target='" + type + id + "'>" + id + "</a>&#39;");
         }
      }
   }

 drawPopup(string.split("&gt;&lt;").join("&gt;<br>&lt;").split("<br>&lt;tag").join("<br>&nbsp;&nbsp;&lt;tag"));
}


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Server handling ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//
// Define an AJAX request object
//

function ajaxGET(url,success,failure,state)
{
 var ajaxRequest=new XMLHttpRequest();

 function ajaxGOT(options) {
  if(this.readyState==4)
     if(this.status==200)
       { if(typeof(options.success)=="function") options.success(this,options.state); }
     else
       { if(typeof(options.failure)=="function") options.failure(this,options.state); }
 }

 ajaxRequest.onreadystatechange = function(){ ajaxGOT.call(ajaxRequest,{success: success, failure: failure, state: state}); };
 ajaxRequest.open("GET", url, true);
 ajaxRequest.send(null);
}


//
// Display the status
//

function displayStatus(type,subtype,content)
{
 var child=document.getElementById("result_status").firstChild;

 do
   {
    if(child.id !== undefined)
       child.style.display="none";

    child=child.nextSibling;
   }
 while(child !== null);

 var chosen_status=document.getElementById("result_status_" + type);

 chosen_status.style.display="";

 if(subtype !== undefined)
   {
    var format_status=document.getElementById("result_status_" + subtype).innerHTML;

    chosen_status.innerHTML=format_status.replace("#",String(content));
   }
}


//
// Display data statistics
//

function displayStatistics()
{
 // Use AJAX to get the statistics

 ajaxGET("fixme.cgi?statistics=yes", runStatisticsSuccess);
}


//
// Success in running data statistics generation.
//

function runStatisticsSuccess(response)
{
 document.getElementById("statistics_data").innerHTML="<pre>" + response.responseText + "</pre>";
 document.getElementById("statistics_link").style.display="none";
}


//
// Get the requested data
//

function displayData(datatype)  // called from fixme.html
{
 // Delete the old data

 vectorData=[];

 unselectFeature();

 select.deactivate();

 layerVectors.destroyFeatures();

 // Print the status

 displayStatus("no_data");

 // Return if just here to clear the data

 if(datatype === "")
    return;

 displayMoreData();
}


function displayMoreData()
{
 // Get the new data

 var mapbounds=map.getExtent().clone();
 mapbounds.transform(epsg900913,epsg4326);

 var url="fixme.cgi";

 url=url + "?lonmin=" + format5f(mapbounds.left);
 url=url + ";latmin=" + format5f(mapbounds.bottom);
 url=url + ";lonmax=" + format5f(mapbounds.right);
 url=url + ";latmax=" + format5f(mapbounds.top);
 url=url + ";data=fixmes";

 // Use AJAX to get the data

 ajaxGET(url, runFixmeSuccess, runFailure);

 updateURLs(true);
}


//
// Success in getting the error log data
//

function runFixmeSuccess(response)
{
 var lines=response.responseText.split("\n");

 var style = new OpenLayers.Style({},{stroke: false,
                                      pointRadius: 3,fillColor: "#FF0000",
                                      cursor: "pointer"});

 var features=[];

 for(var line=0;line<lines.length;line++)
   {
    var words=lines[line].split(" ");

    if(line === 0)
       continue;
    else if(words[0] !== "")
      {
       var dump=words[0];

       if(vectorData[dump])
          continue;
       else
          vectorData[dump]=1;

       var lat=Number(words[1]);
       var lon=Number(words[2]);

       var lonlat = new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

       features.push(new OpenLayers.Feature.Vector(point,{dump: dump},style));
      }
   }

 select.activate();

 layerVectors.addFeatures(features);

 displayStatus("data","fixme",Object.keys(vectorData).length);
}


//
// Failure in getting data.
//

function runFailure(response)
{
 displayStatus("failed");
}
