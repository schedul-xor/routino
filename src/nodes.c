/***************************************
 $Header: /home/amb/CVS/routino/src/nodes.c,v 1.30 2009-05-29 17:45:24 amb Exp $

 Node data type functions.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2008,2009 Andrew M. Bishop

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************/


#include <stdlib.h>
#include <math.h>

#include "profiles.h"
#include "nodes.h"
#include "segments.h"
#include "ways.h"
#include "functions.h"


/*++++++++++++++++++++++++++++++++++++++
  Load in a node list from a file.

  Nodes* LoadNodeList Returns the node list.

  const char *filename The name of the file to load.
  ++++++++++++++++++++++++++++++++++++++*/

Nodes *LoadNodeList(const char *filename)
{
 void *data;
 Nodes *nodes;

 nodes=(Nodes*)malloc(sizeof(Nodes));

 data=MapFile(filename);

 if(!data)
    return(NULL);

 /* Copy the Nodes structure from the loaded data */

 *nodes=*((Nodes*)data);

 /* Adjust the pointers in the Nodes structure. */

 nodes->data=data;
 nodes->offsets=(index_t*)(data+(off_t)nodes->offsets);
 nodes->nodes=(Node*)(data+(off_t)nodes->nodes);

 return(nodes);
}


/*++++++++++++++++++++++++++++++++++++++
  Find the closest node given its latitude and longitude and optionally profile.

  index_t FindNode Returns the node index.

  Nodes* nodes The set of nodes to search.

  Segments *segments The set of segments to use.

  Ways *ways The set of ways to use.

  float latitude The latitude to look for.

  float longitude The longitude to look for.

  distance_t distance The maximum distance to look, returns the final distance.

  Profile *profile The profile of the mode of transport.
  ++++++++++++++++++++++++++++++++++++++*/

index_t FindNode(Nodes* nodes,Segments *segments,Ways *ways,float latitude,float longitude,distance_t *distance,Profile *profile)
{
 int32_t latbin=lat_long_to_bin(latitude )-nodes->latzero;
 int32_t lonbin=lat_long_to_bin(longitude)-nodes->lonzero;
 int     delta=0,count;
 index_t i,best=NO_NODE;

 /* Start with the bin containing the location, then spiral outwards. */

 do
   {
    int latb,lonb,llbin;

    count=0;
   
    for(latb=latbin-delta;latb<=latbin+delta;latb++)
       for(lonb=lonbin-delta;lonb<=lonbin+delta;lonb++)
         {
          if(abs(latb-latbin)<delta && abs(lonb-lonbin)<delta)
             continue;

          llbin=lonb*nodes->latbins+latb;

          if(llbin<0 || llbin>(nodes->latbins*nodes->lonbins))
             continue;

          if(delta>0)
            {
             float lat1=(float)((nodes->latzero+latb)*LAT_LONG_BIN)/LAT_LONG_SCALE;
             float lon1=(float)((nodes->lonzero+lonb)*LAT_LONG_BIN)/LAT_LONG_SCALE;
             float lat2=(float)((nodes->latzero+latb+1)*LAT_LONG_BIN)/LAT_LONG_SCALE;
             float lon2=(float)((nodes->lonzero+lonb+1)*LAT_LONG_BIN)/LAT_LONG_SCALE;

             if(latb==latbin)
               {
                distance_t dist1=Distance(latitude,lon1,latitude,longitude);
                distance_t dist2=Distance(latitude,lon2,latitude,longitude);

                if(dist1>*distance && dist2>*distance)
                   continue;
               }
             else if(lonb==lonbin)
               {
                distance_t dist1=Distance(lat1,longitude,latitude,longitude);
                distance_t dist2=Distance(lat2,longitude,latitude,longitude);

                if(dist1>*distance && dist2>*distance)
                   continue;
               }
             else
               {
                distance_t dist1=Distance(lat1,lon1,latitude,longitude);
                distance_t dist2=Distance(lat2,lon1,latitude,longitude);
                distance_t dist3=Distance(lat2,lon2,latitude,longitude);
                distance_t dist4=Distance(lat1,lon2,latitude,longitude);

                if(dist1>*distance && dist2>*distance && dist3>*distance && dist4>*distance)
                   continue;
               }
            }

          for(i=nodes->offsets[llbin];i<nodes->offsets[llbin+1];i++)
            {
             float lat=(float)((nodes->latzero+latb)*LAT_LONG_BIN+nodes->nodes[i].latoffset)/LAT_LONG_SCALE;
             float lon=(float)((nodes->lonzero+lonb)*LAT_LONG_BIN+nodes->nodes[i].lonoffset)/LAT_LONG_SCALE;

             distance_t dist=Distance(lat,lon,latitude,longitude);

             if(dist<*distance)
               {
                if(profile)
                  {
                   Segment *segment;

                   /* Decide if this is node is valid for the profile */

                   segment=FirstSegment(segments,nodes,i);

                   do
                     {
                      Way *way=LookupWay(ways,segment->way);

                      if(way->allow&profile->allow)
                         break;

                      segment=NextSegment(segments,segment,i);
                     }
                   while(segment);

                   if(!segment)
                      continue;
                  }

                best=i; *distance=dist;
               }
            }

          count++;
         }

    delta++;
   }
 while(count);

 return(best);
}


/*++++++++++++++++++++++++++++++++++++++
  Get the latitude and longitude associated with a node.

  Nodes *nodes The set of nodes.

  index_t index The node index.

  float *latitude Returns the latitude.

  float *longitude Returns the logitude.
  ++++++++++++++++++++++++++++++++++++++*/

void GetLatLong(Nodes *nodes,index_t index,float *latitude,float *longitude)
{
 Node *node=&nodes->nodes[index];
 int latbin=-1,lonbin=-1;
 int start,end,mid;

 /* Binary search - search key closest below is required.
  *
  *  # <- start  |  Check mid and move start or end if it doesn't match
  *  #           |
  *  #           |  Since an inexact match is wanted we must set end=mid-1
  *  # <- mid    |  or start=mid because we know that mid doesn't match.
  *  #           |
  *  #           |  Eventually either end=start or end=start+1 and one of
  *  # <- end    |  start or end is the wanted one.
  */

 /* Search for longitude */

 start=0;
 end=nodes->lonbins-1;

 do
   {
    mid=(start+end)/2;                                 /* Choose mid point */

    if(nodes->offsets[nodes->latbins*mid]<index)       /* Mid point is too low */
       start=mid;
    else if(nodes->offsets[nodes->latbins*mid]>index)  /* Mid point is too high */
       end=mid-1;
    else                                               /* Mid point is correct */
      {lonbin=mid;break;}
   }
 while((end-start)>1);

 if(lonbin==-1)
   {
    if(nodes->offsets[nodes->latbins*end]>index)
       lonbin=start;
    else
       lonbin=end;
   }

 while(lonbin<nodes->lonbins && nodes->offsets[lonbin*nodes->latbins]==nodes->offsets[(lonbin+1)*nodes->latbins])
    lonbin++;

 /* Search for latitude */

 start=0;
 end=nodes->latbins-1;

 do
   {
    mid=(start+end)/2;                                       /* Choose mid point */

    if(nodes->offsets[lonbin*nodes->latbins+mid]<index)      /* Mid point is too low */
       start=mid;
    else if(nodes->offsets[lonbin*nodes->latbins+mid]>index) /* Mid point is too high */
       end=mid-1;
    else                                                     /* Mid point is correct */
      {latbin=mid;break;}
   }
 while((end-start)>1);

 if(latbin==-1)
   {
    if(nodes->offsets[lonbin*nodes->latbins+end]>index)
       latbin=start;
    else
       latbin=end;
   }

 while(latbin<nodes->latbins && nodes->offsets[lonbin*nodes->latbins+latbin]==nodes->offsets[lonbin*nodes->latbins+latbin+1])
    latbin++;

 /* Return the values */

 *latitude =(float)((nodes->latzero+latbin)*LAT_LONG_BIN+node->latoffset)/LAT_LONG_SCALE;
 *longitude=(float)((nodes->lonzero+lonbin)*LAT_LONG_BIN+node->lonoffset)/LAT_LONG_SCALE;
}
