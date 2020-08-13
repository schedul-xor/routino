////////////////////////////////////////////////////////////////////////////////
///////////////////// Map loader (OpenLayers or Leaflet) ///////////////////////
////////////////////////////////////////////////////////////////////////////////


function map_load(callbacks)
{
 var pending = 1;
 var head = document.getElementsByTagName("head")[0];

 /* Allow selecting between libraries at run-time if an array is provided */

 mapprops.libraries=Array.isArray(mapprops.library);

 if(mapprops.libraries)
   {
    if(location.search.length>1 &&
       location.search.match(/library=(leaflet|openlayers2|openlayers)/) &&
       mapprops.library.indexOf(RegExp.$1)!=-1)
       mapprops.library=RegExp.$1;
    else
       mapprops.library=mapprops.library[0];
   }

 /* Call the callbacks when everything is loaded. */

 function call_callbacks()
 {
  if(!--pending)
     eval(callbacks);
 }

 /* Javascript loader */

 function load_js(url, urls)
 {
  var script = document.createElement("script");
  script.src = url;
  script.type = "text/javascript";

  pending++;

  if( urls === undefined )
     script.onload = call_callbacks;
  else
     script.onload = function() { load_js(urls); call_callbacks(); };

  head.appendChild(script);
 }

 /* CSS loader */

 function load_css(url, urls)
 {
  var link = document.createElement("link");
  link.href = url;
  link.type = "text/css";
  link.rel = "stylesheet";

  head.appendChild(link);

  if( urls !== undefined )
     load_css(urls)
 }

 /* Load the external library and local code */

 if(mapprops.library == "leaflet")
   {
    load_css("../leaflet/leaflet.css");
    load_js ("../leaflet/leaflet.js");

    load_js(location.pathname.replace(/\.html.*/,".leaflet.js"));
   }
 else if(mapprops.library == "openlayers")
   {
    load_css("../openlayers/ol.css", "../openlayers/ol-layerswitcher.css");
    load_js ("../openlayers/ol.js",  "../openlayers/ol-layerswitcher.js");

    load_js(location.pathname.replace(/\.html.*/,".openlayers.js"));
   }
 else if(mapprops.library == "openlayers2")
   {
    load_js("../openlayers2/OpenLayers.js");

    load_js(location.pathname.replace(/\.html.*/,".openlayers2.js"));
   }

 call_callbacks();
}
