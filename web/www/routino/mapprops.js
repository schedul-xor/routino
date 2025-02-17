////////////////////////////////////////////////////////////////////////////////
/////////////////////////// Routino map properties /////////////////////////////
////////////////////////////////////////////////////////////////////////////////

var mapprops={ // contains all properties for the map to be displayed.

 // EDIT THIS below to change the map library (either 'openlayers2', 'openlayers' or 'leaflet').

  //library: "openlayers2",
  //library: "openlayers",
  library: "leaflet",

 //library: ["leaflet", "openlayers", "openlayers2"], // Using a list allows selection via URL argument

 // EDIT THIS above to change the map library (either 'openlayers2', 'openlayers' or 'leaflet').


 // EDIT THIS below to change the visible map limits

  westedge:  122.560700,          // Minimum longitude (degrees)
  eastedge:    154.470900,          // Maximum longitude (degrees)

  southedge:  20.082280,          // Minimum latitude (degrees)
  northedge:  45.802450,          // Maximum latitude (degrees)

  zoomout:       4,          // Minimum zoom
  zoomin:       18,          // Maximum zoom

 // EDIT THIS above to change the visible map limits


 // EDIT THIS below to change the map URL(s) and copyright notices

  mapdata: [
           {
            label: "OpenStreetMap",
            tiles: {
                    url: "http://${s}.tile.openstreetmap.org/${z}/${x}/${y}.png",
                    subdomains: ["a","b","c"]
                   },
            attribution: {
                          data_url:  "http://www.openstreetmap.org/copyright",
                          data_text: "© OpenStreetMap contributors",
                          tile_url:  "http://www.openstreetmap.org/copyright",
                          tile_text: "© OpenStreetMap"
                         }
           }
           ],

 // EDIT THIS above to change the map URL(s) and copyright notices


 // EDIT THIS below to change the map source data editing URL (or leave blank for no link)

  editurl: "http://www.openstreetmap.org/edit",

 // EDIT THIS above to change the map source data editing URL (or leave blank for no link)


 // EDIT THIS below to change the map source data browsing URL (or leave blank for no link)

  browseurl: "http://www.openstreetmap.org/browse",

 // EDIT THIS above to change the map source data browsing URL (or leave blank for no link)


 // EDIT THIS below to change the maximum number of markers to include in the HTML

  maxmarkers: 99

 // EDIT THIS above to change the maximum number of markers to include in the HTML

}; // end of map properties
