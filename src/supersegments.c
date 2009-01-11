/***************************************
 $Header: /home/amb/CVS/routino/src/supersegments.c,v 1.4 2009-01-11 09:42:26 amb Exp $

 Super-Segment data type functions.
 ******************/ /******************
 Written by Andrew M. Bishop

 This file Copyright 2008,2009 Andrew M. Bishop
 It may be distributed under the GNU Public License, version 2, or
 any higher version.  See section COPYING of the GNU Public license
 for conditions under which this file may be redistributed.
 ***************************************/


#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "nodes.h"
#include "ways.h"
#include "segments.h"
#include "functions.h"


/*++++++++++++++++++++++++++++++++++++++
  Select the super-segments from the list of segments.

  NodesMem *ChooseSuperNodes Returns the list of super-nodes.

  Nodes *nodes The nodes.

  Segments *segments The existing segments.

  Ways *ways The ways.
  ++++++++++++++++++++++++++++++++++++++*/

NodesMem *ChooseSuperNodes(Nodes *nodes,Segments *segments,Ways *ways)
{
 int i;
 int count_oneway=0,count_twoway=0;
 node_t node=0;
 NodesMem *supernodes;

 /* Find super-nodes */

 supernodes=NewNodeList();

 for(i=0;i<segments->number;i++)
   {
    Way *way;

    if(i>0 && segments->segments[i].node1!=node)
      {
       if((count_oneway*2+count_twoway)>2)
         {
          Node *oldnode=FindNode(nodes,node);

          AppendNode(supernodes,node,oldnode->latitude,oldnode->longitude);
         }

       count_oneway=0;
       count_twoway=0;
      }

    way=FindWay(ways,segments->segments[i].way);

    if(way->type&Way_ONEWAY)
       count_oneway++;
    else
       count_twoway++;

    node=segments->segments[i].node1;

    if(!(i%10000))
      {
       printf("\rFinding Super-Nodes: Segments=%d Super-Nodes=%d",i,supernodes->number);
       fflush(stdout);
      }
   }

 printf("\rFound Super-Nodes: Segments=%d Super-Nodes=%d  \n",segments->number,supernodes->number);
 fflush(stdout);

 return(supernodes);
}


/*++++++++++++++++++++++++++++++++++++++
  Create the super-segments.

  SegmentsMem *CreateSuperSegments Returns the set of super-segments.

  Nodes *nodes The list of nodes.

  Segments *segments The list of segments.

  Ways *ways The list of ways.

  Nodes *supernodes The list of super-nodes.
  ++++++++++++++++++++++++++++++++++++++*/

SegmentsMem *CreateSuperSegments(Nodes *nodes,Segments *segments,Ways *ways,Nodes *supernodes)
{
 SegmentsMem *supersegments;
 int i,j,k;

 /* Create super-segments */

 supersegments=NewSegmentList();

 for(i=0;i<supernodes->number;i++)
   {
    Results *results;

    results=FindRoutes(nodes,segments,supernodes->nodes[i].id,supernodes);

    for(j=0;j<NBINS_RESULTS;j++)
       for(k=0;k<results->number[j];k++)
          if(FindNode(supernodes,results->results[j][k]->node))
            {
             distance_t distance;
             duration_t duration;
             Segment *segment=AppendSegment(supersegments,supernodes->nodes[i].id,results->results[j][k]->node,0);

             distance=results->results[j][k]->shortest.distance;
             duration=results->results[j][k]->quickest.duration;

             if(distance>(distance_short_t)~0)
               {
                fprintf(stderr,"\nSuper-Segment too long (%d->%d) = %.1f km\n",segment->node1,segment->node2,distance_to_km(distance));
                distance=(distance_short_t)~0;
               }

             if(duration>(duration_short_t)~0)
               {
                fprintf(stderr,"\nSuper-Segment too long (%d->%d) = %.1f mins\n",segment->node1,segment->node2,duration_to_minutes(duration));
                duration=(duration_short_t)~0;
               }

             segment->distance=distance;
             segment->duration=duration;
            }

    FreeResultsList(results);

    if(!(i%1000))
      {
       printf("\rFinding Super-Segments: Super-Nodes=%d Super-Segments=%d",i,supersegments->number);
       fflush(stdout);
      }
   }

 printf("\rFound Super-Segments: Super-Nodes=%d Super-Segments=%d  \n",supernodes->number,supersegments->number);
 fflush(stdout);

 return(supersegments);
}
