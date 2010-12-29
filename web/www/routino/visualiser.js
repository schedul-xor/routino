//
// Routino data visualiser web page Javascript
//
// Part of the Routino routing software.
//
// This file Copyright 2008,2009 Andrew M. Bishop
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

var data_types={
                junctions: true,
                super: true,
                oneway: true,
                speed: true,
                weight: true,
                height: true,
                width: true,
                length: true
               };


//
// Junction styles
//

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

var junction_styles={};


//
// Super styles
//

var super_node_style,super_segment_style;


//
// Oneway styles
//

var hex={0: "00", 1: "11",  2: "22",  3: "33",  4: "44",  5: "55",  6: "66",  7: "77",
         8: "88", 9: "99", 10: "AA", 11: "BB", 12: "CC", 13: "DD", 14: "EE", 15: "FF"};


//
// Map configuration
//

var map;
var layerMapnik, layerVectors, layerBoxes;
var epsg4326, epsg900913;

var box;


// 
// Initialise the 'map' object
//

function map_init(lat,lon,zoom)
{
 var data;

 for(data in data_types)
    hideshow_hide(data);

 //
 // Create the map
 //

 epsg4326=new OpenLayers.Projection("EPSG:4326");
 epsg900913=new OpenLayers.Projection("EPSG:900913");

 // UK coordinate range: West -11.0, South 49.5, East 2.0, North 61.0

 // EDIT THIS to change the visible map boundary.
 var mapbounds=new OpenLayers.Bounds(-11.0,49.5,2.0,61.0).transform(epsg4326,epsg900913);

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

                            // EDIT THIS to set the minimum zoom level
                            minZoomLevel: 4,

                            // EDIT THIS to set the number of zoom levels
                            numZoomLevels: 12, // zoom levels 4-15 inclusive

                            // EDIT THIS if you change the minimum zoom level above
                            maxResolution: 156543.0339 / Math.pow(2,4), // Math.pow(2,minZoomLevel)

                            maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34),

                            restrictedExtent: mapbounds,

                            units: "m"
                           });

 map.events.register("moveend", map, mapMoved);

 // Add a Mapnik layer

 layerMapnik = new OpenLayers.Layer.TMS("OSM (Mapnik)",
                                        // EDIT THIS to set the source of map tiles
                                        "http://tile.openstreetmap.org/",
                                        {
                                         // EDIT THIS if you change the source of map tiles above
                                         emptyUrl: "http://openstreetmap.org/openlayers/img/404.png",
                                         type: 'png',
                                         getURL: limitedUrl,
                                         displayOutsideMaxExtent: true,
                                         buffer: 1
                                        });
 map.addLayer(layerMapnik);

 // Get a URL for the tile; limited to mapbounds.

 function limitedUrl(bounds)
 {
  var z = map.getZoom() + map.minZoomLevel;

  if (z>7 && (bounds.right  < mapbounds.left ||
              bounds.left   > mapbounds.right ||
              bounds.top    < mapbounds.bottom ||
              bounds.bottom > mapbounds.top))
     return this.emptyUrl;

  var res = map.getResolution();
  var y = Math.round((this.maxExtent.top - bounds.top) / (res * this.tileSize.h));
  var limit = Math.pow(2, z);

  if (y < 0 || y >= limit)
    return this.emptyUrl;

  var x = Math.round((bounds.left - this.maxExtent.left) / (res * this.tileSize.w));

  x = ((x % limit) + limit) % limit;
  return this.url + z + "/" + x + "/" + y + "." + this.type;
 }

 map.setCenter(mapbounds.getCenterLonLat(), map.getZoomForExtent(mapbounds,true));
 map.maxResolution = map.getResolution();

 // Add a vectors layer
 
 layerVectors = new OpenLayers.Layer.Vector("Markers");
 map.addLayer(layerVectors);

 var colour;
 for(colour in junction_colours)
    junction_styles[colour]=new OpenLayers.Style({},{stroke: false, pointRadius: 2,fillColor: junction_colours[colour]});

 super_node_style   =new OpenLayers.Style({},{stroke: false, pointRadius: 3,fillColor  : "#FF0000"});
 super_segment_style=new OpenLayers.Style({},{fill: false  , strokeWidth: 2,strokeColor: "#FF0000"});

 // Add a boxes layer

 layerBoxes = new OpenLayers.Layer.Boxes("Boxes");
 map.addLayer(layerBoxes);

 box=null;

 // Move the map

 if(lon != 'lon' && lat != 'lat' && zoom != 'zoom')
   {
    var lonlat = new OpenLayers.LonLat(lon,lat).transform(epsg4326,map.getProjectionObject());

    map.moveTo(lonlat,zoom-map.minZoomLevel);
   }
}


//
// Map has moved
//

function mapMoved()
{
 var centre = map.getCenter().clone();

 var lonlat = centre.transform(map.getProjectionObject(),epsg4326);

 var zoom = this.getZoom() + map.minZoomLevel;

 var router_url=document.getElementById("router_url");
 var link_url  =document.getElementById("link_url");
 var edit_url  =document.getElementById("edit_url");

 var args="lat=" + lonlat.lat + ";lon=" + lonlat.lon + ";zoom=" + zoom;

 router_url.href="customrouter.cgi?" + args;
 link_url.href="customvisualiser.cgi?" + args;
 edit_url.href="http://www.openstreetmap.org/edit?" + args;
}


//
// Display data statistics
//

function displayStatistics()
{
 // Use AJAX to get the statistics

 OpenLayers.loadURL("statistics.cgi",null,null,runStatisticsSuccess);

 return(false);
}


//
// Success in running router.
//

function runStatisticsSuccess(response)
{
 var statistics_data=document.getElementById("statistics_data");
 var statistics_link=document.getElementById("statistics_link");

 statistics_data.innerHTML="<pre>" + response.responseText + "</pre>";

 statistics_link.style.display="none";
}


//
// Get the requested data
//

function displayData(datatype)
{
 var data;

 for(data in data_types)
    hideshow_hide(data);

 if(datatype != "")
    hideshow_show(datatype);

 // Delete the old data

 layerVectors.destroyFeatures();

 if(box != null)
    layerBoxes.removeMarker(box);
 box=null;

 // Print the status

 var div_status=document.getElementById("result_status");
 div_status.innerHTML = "No data displayed";

 // Return if just here to clear the data

 if(datatype == "")
    return;

 // Get the new data

 var mapbounds=map.getExtent().clone();
 mapbounds.transform(epsg900913,epsg4326);

 var url="visualiser.cgi";

 url=url + "?lonmin=" + mapbounds.left;
 url=url + ";latmin=" + mapbounds.bottom;
 url=url + ";lonmax=" + mapbounds.right;
 url=url + ";latmax=" + mapbounds.top;
 url=url + ";data=" + datatype;

 // Print the status

 div_status.innerHTML = "Fetching " + datatype + " data ...";

 // Use AJAX to get the data

 switch(datatype)
   {
   case 'junctions':
    OpenLayers.loadURL(url,null,null,runJunctionsSuccess,runFailure);
    break;
   case 'super':
    OpenLayers.loadURL(url,null,null,runSuperSuccess,runFailure);
    break;
   case 'oneway':
    OpenLayers.loadURL(url,null,null,runOnewaySuccess,runFailure);
    break;
   case 'speed':
   case 'weight':
   case 'height':
   case 'width':
   case 'length':
    OpenLayers.loadURL(url,null,null,runLimitSuccess,runFailure);
    break;
   }
}


//
// Success in getting the junctions.
//

function runJunctionsSuccess(response)
{
 var lines=response.responseText.split('\n');

 var div_status=document.getElementById("result_status");
 div_status.innerHTML = "Processing " + (lines.length-2) + " junctions ...";

 var features=[];

 for(line in lines)
   {
    var words=lines[line].split(/ /g);

    if(line == 0)
      {
       var lat1=words[0];
       var lon1=words[1];
       var lat2=words[2];
       var lon2=words[3];

       var bounds = new OpenLayers.Bounds(lon1,lat1,lon2,lat2).transform(epsg4326,map.getProjectionObject());

       box = new OpenLayers.Marker.Box(bounds);

       layerBoxes.addMarker(box);
      }
    else if(words[0] != "")
      {
       var lat=words[0];
       var lon=words[1];
       var count=words[2];

       var lonlat= new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

       features.push(new OpenLayers.Feature.Vector(point,{},junction_styles[count]));

      }
   }

 layerVectors.addFeatures(features);

 div_status.innerHTML = "Processed " + (lines.length-2) + " junctions";
}


//
// Success in getting the super-node and super-segments
//

function runSuperSuccess(response)
{
 var lines=response.responseText.split('\n');

 var div_status=document.getElementById("result_status");
 div_status.innerHTML = "Processing " + (lines.length-2) + " super-nodes/segments ...";

 var features=[];

 var nodepoint;

 for(line in lines)
   {
    var words=lines[line].split(/ /g);

    if(line == 0)
      {
       var lat1=words[0];
       var lon1=words[1];
       var lat2=words[2];
       var lon2=words[3];

       var bounds = new OpenLayers.Bounds(lon1,lat1,lon2,lat2).transform(epsg4326,map.getProjectionObject());

       box = new OpenLayers.Marker.Box(bounds);

       layerBoxes.addMarker(box);
      }
    else if(words[0] != "")
      {
       var lat=words[0];
       var lon=words[1];
       var type=words[2];

       var lonlat= new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

       if(type == "n")
         {
          nodepoint=point;

          features.push(new OpenLayers.Feature.Vector(point,{},super_node_style));
         }
       else
         {
          var line = new OpenLayers.Geometry.LineString([nodepoint,point]);

          features.push(new OpenLayers.Feature.Vector(line,{},super_segment_style));
         }
      }
   }

 layerVectors.addFeatures(features);

 div_status.innerHTML = "Processed " + (lines.length-2) + " super-nodes/segments";
}


//
// Success in getting the oneway data
//

function runOnewaySuccess(response)
{
 var lines=response.responseText.split('\n');

 var div_status=document.getElementById("result_status");
 div_status.innerHTML = "Processing " + (lines.length-2) + " oneway segments ...";

 var features=[];

 for(line in lines)
   {
    var words=lines[line].split(/ /g);

    if(line == 0)
      {
       var lat1=words[0];
       var lon1=words[1];
       var lat2=words[2];
       var lon2=words[3];

       var bounds = new OpenLayers.Bounds(lon1,lat1,lon2,lat2).transform(epsg4326,map.getProjectionObject());

       box = new OpenLayers.Marker.Box(bounds);

       layerBoxes.addMarker(box);
      }
    else if(words[0] != "")
      {
       var lat1=words[0];
       var lon1=words[1];
       var lat2=words[2];
       var lon2=words[3];

       var lonlat1= new OpenLayers.LonLat(lon1,lat1).transform(epsg4326,epsg900913);
       var lonlat2= new OpenLayers.LonLat(lon2,lat2).transform(epsg4326,epsg900913);

     //var point1 = new OpenLayers.Geometry.Point(lonlat1.lon,lonlat1.lat);
       var point2 = new OpenLayers.Geometry.Point(lonlat2.lon,lonlat2.lat);

       var dlat = lonlat2.lat-lonlat1.lat;
       var dlon = lonlat2.lon-lonlat1.lon;
       var dist = Math.sqrt(dlat*dlat+dlon*dlon)/10;
       var ang  = Math.atan2(dlat,dlon);

       var point3 = new OpenLayers.Geometry.Point(lonlat1.lon+dlat/dist,lonlat1.lat-dlon/dist);
       var point4 = new OpenLayers.Geometry.Point(lonlat1.lon-dlat/dist,lonlat1.lat+dlon/dist);

       var line = new OpenLayers.Geometry.LineString([point2,point3,point4,point2]);

       var r=Math.round(7.5+7.9*Math.cos(ang));
       var g=Math.round(7.5+7.9*Math.cos(ang+2.0943951));
       var b=Math.round(7.5+7.9*Math.cos(ang-2.0943951));
       var colour = "#" + hex[r] + hex[g] + hex[b];

       var style=new OpenLayers.Style({},{strokeWidth: 2,strokeColor: colour});

       features.push(new OpenLayers.Feature.Vector(line,{},style));
      }
   }

 layerVectors.addFeatures(features);

 div_status.innerHTML = "Processed " + (lines.length-2) + " oneway segments";
}


//
// Success in getting the speed/weight/height/width/length limits
//

function runLimitSuccess(response)
{
 var lines=response.responseText.split('\n');

 var div_status=document.getElementById("result_status");
 div_status.innerHTML = "Processing " + (lines.length-2) + " limits ...";

 var features=[];

 var nodelonlat;

 for(line in lines)
   {
    var words=lines[line].split(/ /g);

    if(line == 0)
      {
       var lat1=words[0];
       var lon1=words[1];
       var lat2=words[2];
       var lon2=words[3];

       var bounds = new OpenLayers.Bounds(lon1,lat1,lon2,lat2).transform(epsg4326,map.getProjectionObject());

       box = new OpenLayers.Marker.Box(bounds);

       layerBoxes.addMarker(box);
      }
    else if(words[0] != "")
      {
       var lat=words[0];
       var lon=words[1];
       var number=words[2];

       var lonlat= new OpenLayers.LonLat(lon,lat).transform(epsg4326,epsg900913);

       if(number == undefined)
         {
          var point = new OpenLayers.Geometry.Point(lonlat.lon,lonlat.lat);

          nodelonlat=lonlat;

          features.push(new OpenLayers.Feature.Vector(point,{},junction_styles[1]));
         }
       else
         {
          var dlat = lonlat.lat-nodelonlat.lat;
          var dlon = lonlat.lon-nodelonlat.lon;
          var dist = Math.sqrt(dlat*dlat+dlon*dlon)/60;

          var point = new OpenLayers.Geometry.Point(nodelonlat.lon+dlon/dist,nodelonlat.lat+dlat/dist);

          features.push(new OpenLayers.Feature.Vector(point,{},
                                                      new OpenLayers.Style({},{externalGraphic: 'icons/limit-' + number + '.png',
                                                                               graphicYOffset: -9,
                                                                               graphicWidth: 19,
                                                                               graphicHeight: 19})));
         }
      }
   }

 layerVectors.addFeatures(features);

 div_status.innerHTML = "Processed " + (lines.length-2) + " limits";
}


//
// Failure in getting data.
//

function runFailure(response)
{
 var div_status=document.getElementById("result_status");
 div_status.innerHTML = "Failed to get visualiser data!";

 window.alert("Failed to get visualiser data!\n" + response.statusText);
}
