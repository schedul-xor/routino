//
// Routino data visualiser web page Javascript
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


//
// Data types
//

var data_types=[
                "junctions",
                "super",
                "waytype",
                "highway",
                "transport",
                "barrier",
                "turns",
                "speed",
                "weight",
                "height",
                "width",
                "length",
                "property",
                "errorlogs"
               ];


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Initialisation /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Process the URL query string and extract the arguments

var legal={"^lon"     : "^[-0-9.]+$",
           "^lat"     : "^[-0-9.]+$",
           "^zoom"    : "^[-0-9.]+$",
           "^data"    : "^.+$",
           "^subdata" : "^.+$"};

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
var displaytype="", displaysubtype="";

//
// Initialise the 'map' object
//

function map_init()             // called from visualiser.html
{
 // Create the map (Map URLs and limits are in mapprops.js)

 var extent = ol.extent.boundingExtent([ol.proj.fromLonLat([mapprops.westedge,mapprops.southedge]),
                                        ol.proj.fromLonLat([mapprops.eastedge,mapprops.northedge])]);

 map = new ol.Map({target: "map",
                   view: new ol.View({minZoom: mapprops.zoomout,
                                      maxZoom: mapprops.zoomin,
                                      extent: extent}),
                   controls: []
                 });

 // Add map tile layers

 for(var l=mapprops.mapdata.length-1; l>=0; l--)
   {
    mapprops.mapdata[l].tiles.url=mapprops.mapdata[l].tiles.url.replace(/\$\{/g,"{");
    var urls;

    if(mapprops.mapdata[l].tiles.subdomains===undefined)
       urls=[mapprops.mapdata[l].tiles.url];
    else
      {
       urls=[];

       for(var s=0; s<mapprops.mapdata[l].tiles.subdomains.length; s++)
          urls.push(mapprops.mapdata[l].tiles.url.replace(/\{s}/,mapprops.mapdata[l].tiles.subdomains[s]));
      }

    layerMap[l] = new ol.layer.Tile({title: mapprops.mapdata[l].label,
                                     type: 'base',
                                     source: new ol.source.XYZ({urls: urls}),
                                     visible: l?false:true});

    map.addLayer(layerMap[l]);
   }

 // Add the controls

 map.addControl(new ol.control.Zoom());
 map.addControl(new ol.control.ScaleLine());
 map.addControl(new ol.control.LayerSwitcher());

 // Update the attribution if the layer changes

 function change_attribution_event(event)
 {
  for(var l=0; l<mapprops.mapdata.length; l++)
     if(layerMap[l].getVisible())
        change_attribution(l);
 }

 map.getLayerGroup().on("change", change_attribution_event);

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

 layerHighlights = new ol.layer.Vector({source: new ol.source.Vector()});

 map.addLayer(layerHighlights);

 layerVectors = new ol.layer.Vector({source: new ol.source.Vector()});

 map.addLayer(layerVectors);

 // Handle feature selection and popup

 map.on("click", function(e) { var first=true; map.forEachFeatureAtPixel(e.pixel, function (feature, layer) { if(first) selectFeature(feature); first=false; }) });

 createPopup();

 // Move the map

 map.on("moveend", (function() { displayMoreData(); updateURLs(false);}), map);

 var lon =args["lon"];
 var lat =args["lat"];
 var zoom=args["zoom"];

 if(lon !== undefined && lat !== undefined && zoom !== undefined)
   {
    lat  = Number(lat);
    lon  = Number(lon);
    zoom = Number(zoom);

    if(lon<mapprops.westedge) lon=mapprops.westedge;
    if(lon>mapprops.eastedge) lon=mapprops.eastedge;

    if(lat<mapprops.southedge) lat=mapprops.southedge;
    if(lat>mapprops.northedge) lat=mapprops.northedge;

    if(zoom<mapprops.zoomout) zoom=mapprops.zoomout;
    if(zoom>mapprops.zoomin)  zoom=mapprops.zoomin;

    map.getView().setCenter(ol.proj.fromLonLat([lon,lat]));
    map.getView().setZoom(zoom);
   }
 else
    map.getView().fit(extent,map.getSize());

 // Unhide editing URL if variable set

 if(mapprops.editurl !== undefined && mapprops.editurl !== "")
   {
    var edit_url=document.getElementById("edit_url");

    edit_url.style.display="";
    edit_url.href=mapprops.editurl;
   }

 // Select the data view if selected

 var datatype=args["data"];
 var datasubtype=args["subdata"];

 if(datatype !== undefined)
    displayData(datatype, datasubtype);

 // Update the URL

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
 var lonlat = ol.proj.toLonLat(map.getView().getCenter());

 var zoom = map.getView().getZoom();

 if( ! Number.isInteger(zoom) )
    zoom = format5f(zoom);

 return "lat=" + format5f(lonlat[1]) + ";lon=" + format5f(lonlat[0]) + ";zoom=" + zoom;
}


//
// Update the URLs
//

function updateURLs(addhistory)
{
 var mapargs=buildMapArguments();
 var dataargs=";data=" + displaytype;
 var libargs=";library=" + mapprops.library;

 if(displaytype === "")
    dataargs="";
 else if(displaysubtype !== "")
    dataargs+=";subdata=" + displaysubtype;

 if(!mapprops.libraries)
    libargs="";

 var links=document.getElementsByTagName("a");

 for(var i=0; i<links.length; i++)
   {
    var element=links[i];

    if(element.id == "permalink_url")
       element.href=location.pathname + "?" + mapargs + dataargs + libargs;

    if(element.id == "router_url")
       if(location.pathname.match(/visualiser\.html\.([a-zA-Z-]+)$/))
          element.href="router.html" + "." + RegExp.$1 + "?" + mapargs + libargs;
       else
          element.href="router.html" + "?" + mapargs + libargs;

    if(element.id == "edit_url")
       element.href=mapprops.editurl + "?" + mapargs;

    if(element.id.match(/^lang_([a-zA-Z-]+)_url$/))
       element.href="visualiser.html" + "." + RegExp.$1 + "?" + mapargs + dataargs + libargs;
   }

 if(addhistory)
    history.replaceState(null, null, location.pathname + "?" + mapargs + dataargs + libargs);
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
 if(feature.get('dump'))
    ajaxGET("visualiser.cgi?dump=" + feature.get('dump'), runDumpSuccess);

 layerHighlights.getSource().clear();

 var style;

 if(feature.get('highlight'))
    style = feature.get('highlight');
 else
    style = new ol.style.Style({stroke: new ol.style.Stroke({width: 8, color: "#F0F000"}),
                                image: new ol.style.Circle({fill: new ol.style.Fill({color: "#F0F000"}),
                                                            radius: 5})});

 var highlight = new ol.Feature(feature.getGeometry());

 highlight.setStyle(style);

 layerHighlights.getSource().addFeature(highlight);
}


//
// Un-select a feature
//

function unselectFeature(feature)
{
 layerHighlights.getSource().clear();

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
    var Types=["Node", "Way", "Relation"];

    for(var t in types)
      {
       var Type=Types[t];
       var type=types[t];

       var regexp=RegExp(Type + " [0-9]+");

       var match;

       while((match=string.match(regexp)) !== null)
         {
          match=String(match);

          var id=match.slice(1+type.length,match.length);

          string=string.replace(regexp,Type + " <a href='" + mapprops.browseurl + "/" + type + "/" + id + "' target='" + type + id + "'>" + id + "</a>");
         }
      }
   }

 drawPopup(string.split("\n").join("<br>"));
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

 ajaxGET("statistics.cgi", runStatisticsSuccess);
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

function displayData(datatype, datasubtype)  // called from visualiser.html
{
 // Display the form entry

 for(var data in data_types)
    hideshow_hide(data_types[data]);

 if(datatype !== "")
    hideshow_show(datatype);

 if(datasubtype === undefined)
    datasubtype="";

 // Delete the old data

 vectorData=[];

 unselectFeature();

 layerVectors.getSource().clear();

 // Print the status

 displayStatus("no_data");

 // Return if just here to clear the data

 if(datatype === "")
   {
    displaytype = "";
    displaysubtype = "";
    updateURLs(true);
    return;
   }

 // Determine the type of data

 switch(datatype)
   {
   case "junctions":
    break;
   case "super":
    break;
   case "waytype":
    var waytypes=document.forms["waytypes"].elements["waytype"];
    for(var w in waytypes)
       if(datasubtype == waytypes[w].value)
          waytypes[w].checked=true;
       else if(waytypes[w].checked)
          datasubtype=waytypes[w].value;
    break;
   case "highway":
    var highways=document.forms["highways"].elements["highway"];
    for(var h in highways)
       if(datasubtype == highways[h].value)
          highways[h].checked=true;
       else if(highways[h].checked)
          datasubtype=highways[h].value;
    break;
   case "transport":
    var transports=document.forms["transports"].elements["transport"];
    for(var t in transports)
       if(datasubtype == transports[t].value)
          transports[t].checked=true;
       else if(transports[t].checked)
          datasubtype=transports[t].value;
    break;
   case "barrier":
    var transports=document.forms["barriers"].elements["barrier"];
    for(var t in transports)
       if(datasubtype == transports[t].value)
          transports[t].checked=true;
       else if(transports[t].checked)
          datasubtype=transports[t].value;
    break;
   case "turns":
    break;
   case "speed":
   case "weight":
   case "height":
   case "width":
   case "length":
    break;
   case "property":
    var properties=document.forms["properties"].elements["property"];
    for(var p in properties)
       if(datasubtype == properties[p].value)
          properties[p].checked=true;
       else if(properties[p].checked)
          datasubtype=properties[p].value;
    break;
   case "errorlogs":
    break;
   }

 // Update the URLs

 displaytype = datatype;
 displaysubtype = datasubtype;

 displayMoreData();
}


function displayMoreData()
{
 // Get the new data

 var mapbounds=map.getView().calculateExtent(map.getSize());

 var url="visualiser.cgi";

 url=url + "?lonmin=" + format5f(ol.proj.toLonLat([mapbounds[0],mapbounds[1]])[0]);
 url=url + ";latmin=" + format5f(ol.proj.toLonLat([mapbounds[0],mapbounds[1]])[1]);
 url=url + ";lonmax=" + format5f(ol.proj.toLonLat([mapbounds[2],mapbounds[3]])[0]);
 url=url + ";latmax=" + format5f(ol.proj.toLonLat([mapbounds[2],mapbounds[3]])[1]);
 url=url + ";data=" + displaytype;

 // Use AJAX to get the data

 switch(displaytype)
   {
   case "junctions":
    ajaxGET(url, runJunctionsSuccess, runFailure);
    break;
   case "super":
    ajaxGET(url, runSuperSuccess, runFailure);
    break;
   case "waytype":
    url+="-" + displaysubtype;
    ajaxGET(url, runWaytypeSuccess, runFailure);
    break;
   case "highway":
    url+="-" + displaysubtype;
    ajaxGET(url, runHighwaySuccess, runFailure);
    break;
   case "transport":
    url+="-" + displaysubtype;
    ajaxGET(url, runTransportSuccess, runFailure);
    break;
   case "barrier":
    url+="-" + displaysubtype;
    ajaxGET(url, runBarrierSuccess, runFailure);
    break;
   case "turns":
    ajaxGET(url, runTurnsSuccess, runFailure);
    break;
   case "speed":
   case "weight":
   case "height":
   case "width":
   case "length":
    ajaxGET(url, runLimitSuccess, runFailure);
    break;
   case "property":
    url+="-" + displaysubtype;
    ajaxGET(url, runPropertySuccess, runFailure);
    break;
   case "errorlogs":
    ajaxGET(url, runErrorlogSuccess, runFailure);
    break;
   }

 updateURLs(true);
}


//
// Success in getting the junctions.
//

function runJunctionsSuccess(response)
{
 var lines=response.responseText.split("\n");

 var junction_colours={
                       0: "#FFFFFF",
                       1: "#FF0000",
                       2: "#FFFF00",
                       3: "#00FF00",
                       4: "#8B4513",
                       5: "#00BFFF",
                       6: "#FF69B4",
                       7: "#000000",
                       8: "#000000",
                       9: "#000000"
                      };

 var styles={};

 for(var colour in junction_colours)
    styles[colour] = new ol.style.Style({image: new ol.style.Circle({fill: new ol.style.Fill({color: junction_colours[colour]}),
                                                                     radius: 2})});

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
       var count=words[3];

       var geometry = new ol.geom.Point(ol.proj.fromLonLat([lon,lat]));

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(styles[count]);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","junctions",Object.keys(vectorData).length);
}


//
// Success in getting the super-node and super-segments
//

function runSuperSuccess(response)
{
 var lines=response.responseText.split("\n");

 var node_style = new ol.style.Style({image: new ol.style.Circle({fill: new ol.style.Fill({color: "#FF0000"}),
                                                                  radius: 4})});

 var highlight_style = new ol.style.Style({image: new ol.style.Circle({fill: new ol.style.Fill({color: "#F0F000"}),
                                                                       radius: 8})});

 var segment_style = new ol.style.Style({stroke: new ol.style.Stroke({width: 2, color: "#FF0000"})});

 var features=[];

 var nodelonlat;

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

       if(dump.charAt(0) == "n")
         {
          nodelonlat = ol.proj.fromLonLat([lon,lat]);

          var geometry = new ol.geom.Point(nodelonlat);

          var feature = new ol.Feature({geometry: geometry, dump: dump, highlight: highlight_style});

          feature.setStyle(node_style);

          features.push(feature);
         }
       else
         {
          var geometry = new ol.geom.LineString([ol.proj.fromLonLat([lon,lat]),nodelonlat]);

          var feature = new ol.Feature({geometry: geometry, dump: dump});

          feature.setStyle(segment_style);

          features.push(feature);
         }
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","super",Object.keys(vectorData).length);
}


//
// Success in getting the waytype data
//

function runWaytypeSuccess(response)
{
 var hex={0: "00", 1: "11",  2: "22",  3: "33",  4: "44",  5: "55",  6: "66",  7: "77",
          8: "88", 9: "99", 10: "AA", 11: "BB", 12: "CC", 13: "DD", 14: "EE", 15: "FF"};

 var lines=response.responseText.split("\n");

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

       var lat1=Number(words[1]);
       var lon1=Number(words[2]);
       var lat2=Number(words[3]);
       var lon2=Number(words[4]);

       var lonlat1 = ol.proj.fromLonLat([lon1,lat1]);
       var lonlat2 = ol.proj.fromLonLat([lon2,lat2]);

       var dlat = lonlat2[1]-lonlat1[1];
       var dlon = lonlat2[0]-lonlat1[0];
       var dist = Math.sqrt(dlat*dlat+dlon*dlon)/10;
       var ang  = Math.atan2(dlat,dlon);

       var lonlat3 = [lonlat1[0]+dlat/dist,lonlat1[1]-dlon/dist];
       var lonlat4 = [lonlat1[0]-dlat/dist,lonlat1[1]+dlon/dist];

       var r=Math.round(7.5+7.9*Math.cos(ang));
       var g=Math.round(7.5+7.9*Math.cos(ang+2.0943951));
       var b=Math.round(7.5+7.9*Math.cos(ang-2.0943951));
       var colour = "#" + hex[r] + hex[g] + hex[b];

       var geometry = new ol.geom.LineString([lonlat2,lonlat3,lonlat4,lonlat2]);

       var style = new ol.style.Style({stroke: new ol.style.Stroke({width: 2, color: colour})});

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(style);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","waytype",Object.keys(vectorData).length);
}


//
// Success in getting the highway data
//

function runHighwaySuccess(response)
{
 var lines=response.responseText.split("\n");

 var style = new ol.style.Style({stroke: new ol.style.Stroke({width: 2, color: "#FF0000"})});

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

       var lat1=Number(words[1]);
       var lon1=Number(words[2]);
       var lat2=Number(words[3]);
       var lon2=Number(words[4]);

       var lonlat1 = ol.proj.fromLonLat([lon1,lat1]);
       var lonlat2 = ol.proj.fromLonLat([lon2,lat2]);

       var geometry = new ol.geom.LineString([lonlat1,lonlat2]);

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(style);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","highway",Object.keys(vectorData).length);
}


//
// Success in getting the transport data
//

function runTransportSuccess(response)
{
 var lines=response.responseText.split("\n");

 var style = new ol.style.Style({stroke: new ol.style.Stroke({width: 2, color: "#FF0000"})});

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

       var lat1=Number(words[1]);
       var lon1=Number(words[2]);
       var lat2=Number(words[3]);
       var lon2=Number(words[4]);

       var lonlat1 = ol.proj.fromLonLat([lon1,lat1]);
       var lonlat2 = ol.proj.fromLonLat([lon2,lat2]);

       var geometry = new ol.geom.LineString([lonlat1,lonlat2]);

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(style);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","transport",Object.keys(vectorData).length);
}


//
// Success in getting the barrier data
//

function runBarrierSuccess(response)
{
 var lines=response.responseText.split("\n");

 var style = new ol.style.Style({image: new ol.style.Circle({fill: new ol.style.Fill({color: "#FF0000"}),
                                                             radius: 3})});

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

       var geometry = new ol.geom.Point(ol.proj.fromLonLat([lon,lat]));

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(style);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","barrier",Object.keys(vectorData).length);
}


//
// Success in getting the turn restrictions data
//

function runTurnsSuccess(response)
{
 var lines=response.responseText.split("\n");

 var style = new ol.style.Style({stroke: new ol.style.Stroke({width: 2, color: "#FF0000"})});

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

       var lat1=Number(words[1]);
       var lon1=Number(words[2]);
       var lat2=Number(words[3]);
       var lon2=Number(words[4]);
       var lat3=Number(words[5]);
       var lon3=Number(words[6]);

       var lonlat1 = ol.proj.fromLonLat([lon1,lat1]);
       var lonlat2 = ol.proj.fromLonLat([lon2,lat2]);
       var lonlat3 = ol.proj.fromLonLat([lon3,lat3]);

       var geometry = new ol.geom.LineString([lonlat1,lonlat2,lonlat3]);

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(style);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","turns",Object.keys(vectorData).length);
}


//
// Success in getting the speed/weight/height/width/length limits
//

function runLimitSuccess(response)
{
 var lines=response.responseText.split("\n");

 var node_style = new ol.style.Style({image: new ol.style.Circle({fill: new ol.style.Fill({color: "#FF0000"}),
                                                                  radius: 3})});

 var segment_style = new ol.style.Style({stroke: new ol.style.Stroke({width: 2, color: "#FF0000"})});

 var features=[];

 var nodelonlat;

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
       var number=words[3];

       var lonlat = ol.proj.fromLonLat([lon,lat]);

       if(number === undefined)
         {
          nodelonlat = lonlat;

          var geometry = new ol.geom.Point(nodelonlat);

          var feature = new ol.Feature({geometry: geometry, dump: dump});

          feature.setStyle(node_style);

          features.push(feature);
         }
       else
         {
          geometry = new ol.geom.LineString([nodelonlat,lonlat]);

          var feature = new ol.Feature({geometry: geometry, dump: dump});

          feature.setStyle(segment_style);

          features.push(feature);

          var dlat = lonlat[1]-nodelonlat[1];
          var dlon = lonlat[0]-nodelonlat[0];
          var dist = Math.sqrt(dlat*dlat+dlon*dlon)/120;

          geometry = new ol.geom.Point([nodelonlat[0]+dlon/dist, nodelonlat[1]+dlat/dist]);

          var style = new ol.style.Style({image: new ol.style.Icon({src: "icons/limit-" + number + ".png",
                                                              anchor: [0.5,0.5]})});

          var feature = new ol.Feature({geometry: geometry, dump: dump});

          feature.setStyle(style);

          features.push(feature);
         }
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","limit",Object.keys(vectorData).length);
}


//
// Success in getting the property data
//

function runPropertySuccess(response)
{
 var lines=response.responseText.split("\n");

 var style = new ol.style.Style({stroke: new ol.style.Stroke({width: 2, color: "#FF0000"})});

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

       var lat1=Number(words[1]);
       var lon1=Number(words[2]);
       var lat2=Number(words[3]);
       var lon2=Number(words[4]);

       var lonlat1 = ol.proj.fromLonLat([lon1,lat1]);
       var lonlat2 = ol.proj.fromLonLat([lon2,lat2]);

       var geometry = new ol.geom.LineString([lonlat1,lonlat2]);

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(style);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","property",Object.keys(vectorData).length);
}


//
// Success in getting the error log data
//

function runErrorlogSuccess(response)
{
 var lines=response.responseText.split("\n");

 var style = new ol.style.Style({image: new ol.style.Circle({fill: new ol.style.Fill({color: "#FF0000"}),
                                                             radius: 3})});

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

       var geometry = new ol.geom.Point(ol.proj.fromLonLat([lon,lat]));

       var feature = new ol.Feature({geometry: geometry, dump: dump});

       feature.setStyle(style);

       features.push(feature);
      }
   }

 layerVectors.getSource().addFeatures(features);

 displayStatus("data","errorlogs",Object.keys(vectorData).length);
}


//
// Failure in getting data.
//

function runFailure(response)
{
 displayStatus("failed");
}
