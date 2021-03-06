/***************************************
 OSM file parser (either JOSM or planet)

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2008-2015, 2018 Andrew M. Bishop

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
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "types.h"
#include "typesx.h"

#include "nodesx.h"
#include "waysx.h"
#include "relationsx.h"

#include "osmparser.h"
#include "tagging.h"
#include "logging.h"


/* Macros */

/*+ Checks if a value in the XML is one of the allowed values for true. +*/
#define ISTRUE(xx)  (!strcmp(xx,"true") || !strcmp(xx,"yes") || !strcmp(xx,"1"))

/*+ Checks if a value in the XML is one of the allowed values for false. +*/
#define ISFALSE(xx) (!strcmp(xx,"false") || !strcmp(xx,"no") || !strcmp(xx,"0"))

/* Local parsing variables (re-initialised for each file) */

static NodesX     *nodes;
static WaysX      *ways;
static RelationsX *relations;

static node_t     *way_nodes;
static int         way_nnodes;

static node_t     *relation_nodes;
static int         relation_nnodes;
static way_t      *relation_ways;
static int         relation_nways;
static relation_t *relation_relations;
static int         relation_nrelations;
static way_t       relation_from;
static int         relation_from_count;
static way_t       relation_to;
static int         relation_to_count;
static node_t      relation_via;
static int         relation_via_count;

/* Local parsing functions */

static double parse_speed(way_t id,const char *k,const char *v);
static double parse_weight(way_t id,const char *k,const char *v);
static double parse_length(way_t id,const char *k,const char *v);


/*++++++++++++++++++++++++++++++++++++++
  Initialise the OSM parser by initialising the local variables.

  NodesX *OSMNodes The data structure of nodes to fill in.

  WaysX *OSMWays The data structure of ways to fill in.

  RelationsX *OSMRelations The data structure of relations to fill in.
  ++++++++++++++++++++++++++++++++++++++*/

void InitialiseParser(NodesX *OSMNodes,WaysX *OSMWays,RelationsX *OSMRelations)
{
 /* Copy the function parameters and initialise the variables */

 nodes=OSMNodes;
 ways=OSMWays;
 relations=OSMRelations;

 way_nodes=(node_t*)malloc(256*sizeof(node_t));

 relation_nodes    =(node_t    *)malloc(256*sizeof(node_t));
 relation_ways     =(way_t     *)malloc(256*sizeof(way_t));
 relation_relations=(relation_t*)malloc(256*sizeof(relation_t));
}


/*++++++++++++++++++++++++++++++++++++++
  Clean up the memory after parsing.
  ++++++++++++++++++++++++++++++++++++++*/

void CleanupParser(void)
{
 /* Free the variables */

 free(way_nodes);

 free(relation_nodes);
 free(relation_ways);
 free(relation_relations);
}


/*++++++++++++++++++++++++++++++++++++++
  Add node references to a way.

  int64_t node_id The node ID to add or zero to clear the list.
  ++++++++++++++++++++++++++++++++++++++*/

void AddWayRefs(int64_t node_id)
{
 if(node_id==0)
    way_nnodes=0;
 else
   {
    node_t id;

    if(way_nnodes && (way_nnodes%256)==0)
       way_nodes=(node_t*)realloc((void*)way_nodes,(way_nnodes+256)*sizeof(node_t));

    id=(node_t)node_id;
    logassert((int64_t)id==node_id,"Node ID too large (change node_t to 64-bits?)"); /* check node id can be stored in node_t data type. */

    way_nodes[way_nnodes++]=id;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Add node, way or relation references to a relation.

  int64_t node_id The node ID to add or zero if it is not a node.

  int64_t way_id The way ID to add or zero if it is not a way.

  int64_t relation_id The relation ID to add or zero if it is not a relation.

  const char *role The role played by this referenced item or NULL.

  If all of node_id, way_id and relation_id are zero then the list is cleared.
  ++++++++++++++++++++++++++++++++++++++*/

void AddRelationRefs(int64_t node_id,int64_t way_id,int64_t relation_id,const char *role)
{
 if(node_id==0 && way_id==0 && relation_id==0)
   {
    relation_nnodes=0;
    relation_nways=0;
    relation_nrelations=0;

    relation_from=NO_WAY_ID;
    relation_from_count=0;
    relation_from_count=0;
    relation_to=NO_WAY_ID;
    relation_to_count=0;
    relation_via=NO_NODE_ID;
    relation_via_count=0;
   }
 else if(node_id!=0)
   {
    node_t id;

    id=(node_t)node_id;
    logassert((int64_t)id==node_id,"Node ID too large (change node_t to 64-bits?)"); /* check node id can be stored in node_t data type. */

    if(relation_nnodes && (relation_nnodes%256)==0)
       relation_nodes=(node_t*)realloc((void*)relation_nodes,(relation_nnodes+256)*sizeof(node_t));

    relation_nodes[relation_nnodes++]=id;

    if(role)
      {
       if(!strcmp(role,"via"))
         {
          relation_via_count++;
          if(relation_via==NO_NODE_ID)
             relation_via=id;
         }
      }
   }
 else if(way_id!=0)
   {
    way_t id;

    id=(way_t)way_id;
    logassert((int64_t)id==way_id,"Way ID too large (change way_t to 64-bits?)"); /* check way id can be stored in way_t data type. */

    if(relation_nways && (relation_nways%256)==0)
       relation_ways=(way_t*)realloc((void*)relation_ways,(relation_nways+256)*sizeof(way_t));

    relation_ways[relation_nways++]=id;

    if(role)
      {
       if(!strcmp(role,"from"))
         {
          relation_from_count++;
          if(relation_from==NO_WAY_ID)
             relation_from=id;
         }
       else if(!strcmp(role,"to"))
         {
          relation_to_count++;
          if(relation_to==NO_WAY_ID)
             relation_to=id;
         }
      }
   }
 else /* if(relation_id!=0) */
   {
    relation_t id;

    id=(relation_t)relation_id;
    logassert((int64_t)id==relation_id,"Relation ID too large (change relation_t to 64-bits?)"); /* check relation id can be stored in relation_t data type. */

    if(relation_nrelations && (relation_nrelations%256)==0)
       relation_relations=(relation_t*)realloc((void*)relation_relations,(relation_nrelations+256)*sizeof(relation_t));

    relation_relations[relation_nrelations++]=relation_id;
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Process the tags associated with a node.

  TagList *tags The list of node tags.

  int64_t node_id The id of the node.

  double latitude The latitude of the node.

  double longitude The longitude of the node.

  int mode The mode of operation to take (create, modify, delete).
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessNodeTags(TagList *tags,int64_t node_id,double latitude,double longitude,int mode)
{
 transports_t allow=Transports_ALL;
 nodeflags_t flags=0;
 node_t id;
 int i;

 /* Convert id */

 id=(node_t)node_id;
 logassert((int64_t)id==node_id,"Node ID too large (change node_t to 64-bits?)"); /* check node id can be stored in node_t data type. */

 /* Delete */

 if(mode==MODE_DELETE)
   {
    AppendNodeList(nodes,id,degrees_to_radians(latitude),degrees_to_radians(longitude),allow,NODE_DELETED);

    return;
   }

 /* Parse the tags */

 for(i=0;i<tags->ntags;i++)
   {
    int recognised=0;
    char *k=tags->k[i];
    char *v=tags->v[i];

    switch(*k)
      {
      case 'b':
       if(!strcmp(k,"bicycle"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Bicycle;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'bicycle' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       break;

      case 'f':
       if(!strcmp(k,"foot"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Foot;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'foot' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       break;

      case 'g':
       if(!strcmp(k,"goods"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Goods;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'goods' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       break;

      case 'h':
       if(!strcmp(k,"horse"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Horse;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'horse' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"hgv"))
         {
          if(ISFALSE(v))
             allow&=~Transports_HGV;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'hgv' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       break;

      case 'm':
       if(!strcmp(k,"moped"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Moped;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'moped' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"motorcycle"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Motorcycle;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'motorcycle' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"motorcar"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Motorcar;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'motorcar' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       break;

      case 'p':
       if(!strcmp(k,"psv"))
         {
          if(ISFALSE(v))
             allow&=~Transports_PSV;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'psv' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       break;

      case 'r':
       if(!strcmp(k,"roundabout"))
         {
          if(ISTRUE(v))
             flags|=NODE_MINIRNDBT;
          else
             logerror("Node %"Pnode_t" has an unrecognised tag 'roundabout' = '%s' (after tagging rules); using 'no'.\n",id,v);
          recognised=1; break;
         }

       break;

      case 'w':
       if(!strcmp(k,"wheelchair"))
         {
          if(ISFALSE(v))
             allow&=~Transports_Wheelchair;
          else if(!ISTRUE(v))
             logerror("Node %"Pnode_t" has an unrecognised tag 'wheelchair' = '%s' (after tagging rules); using 'yes'.\n",logerror_node(id),v);
          recognised=1; break;
         }

       break;

      default:
       break;
      }

    if(!recognised)
       logerror("Node %"Pnode_t" has an unrecognised tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_node(id),k,v);
   }

 /* Create the node */

 AppendNodeList(nodes,id,degrees_to_radians(latitude),degrees_to_radians(longitude),allow,flags);
}


/*++++++++++++++++++++++++++++++++++++++
  Process the tags associated with a way.

  TagList *tags The list of way tags.

  int64_t way_id The id of the way.

  int mode The mode of operation to take (create, modify, delete).
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessWayTags(TagList *tags,int64_t way_id,int mode)
{
 Way way={0};
 int oneway=0,area=0;
 int roundabout=0,lanes=0,cyclebothways=0;
 char *name=NULL,*ref=NULL,*refname=NULL;
 way_t id;
 int i;

 /* Convert id */

 id=(way_t)way_id;
 logassert((int64_t)id==way_id,"Way ID too large (change way_t to 64-bits?)"); /* check way id can be stored in way_t data type. */

 /* Delete */

 if(mode==MODE_DELETE || mode==MODE_MODIFY)
   {
    way.type=WAY_DELETED;

    AppendWayList(ways,id,&way,way_nodes,way_nnodes,"");
   }

 if(mode==MODE_DELETE)
    return;

 /* Sanity check */

 if(way_nnodes==0)
   {
    logerror("Way %"Pway_t" has no nodes.\n",logerror_way(id));
    return;
   }

 if(way_nnodes==1)
   {
    logerror_node(way_nodes[0]); /* Extra logerror information since way isn't stored */
    logerror("Way %"Pway_t" has only one node.\n",logerror_way(id));
    return;
   }

 /* Parse the tags - just look for highway */

 for(i=0;i<tags->ntags;i++)
   {
    char *k=tags->k[i];
    char *v=tags->v[i];

    if(!strcmp(k,"highway"))
      {
       way.type=HighwayType(v);

       if(way.type==Highway_None)
          logerror("Way %"Pway_t" has an unrecognised highway type '%s' (after tagging rules); ignoring it.\n",logerror_way(id),v);

       break;
      }
   }

 /* Don't continue if this is not a highway (bypass error logging) */

 if(way.type==Highway_None)
    return;

 /* Parse the tags - look for the others */

 for(i=0;i<tags->ntags;i++)
   {
    int recognised=0;
    char *k=tags->k[i];
    char *v=tags->v[i];

    switch(*k)
      {
      case 'a':
       if(!strcmp(k,"area"))
         {
          if(ISTRUE(v))
             area=1;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'area' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'b':
       if(!strcmp(k,"bicycle"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Bicycle;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'bicycle' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"bicycleroute"))
         {
          if(ISTRUE(v))
             way.props|=Properties_BicycleRoute;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'bicycleroute' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"bridge"))
         {
          if(ISTRUE(v))
             way.props|=Properties_Bridge;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'bridge' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'c':
       if(!strcmp(k,"cyclebothways"))
         {
          if(ISTRUE(v))
             cyclebothways=1;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'cyclebothways' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }
       break;

      case 'f':
       if(!strcmp(k,"foot"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Foot;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'foot' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"footroute"))
         {
          if(ISTRUE(v))
             way.props|=Properties_FootRoute;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'footroute' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'g':
       if(!strcmp(k,"goods"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Goods;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'goods' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'h':
       if(!strcmp(k,"highway"))
         {recognised=1; break;}

       if(!strcmp(k,"horse"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Horse;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'horse' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"hgv"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_HGV;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'hgv' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'l':
       if(!strcmp(k,"lanes"))
         {
          int en=0;
          float lanesf;
          if(sscanf(v,"%f%n",&lanesf,&en)==1 && en && !v[en])
             lanes=(int)lanesf;
          else
             logerror("Way %"Pway_t" has an unrecognised tag 'lanes' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'm':
       if(!strncmp(k,"max",3))
         {
          if(!strcmp(k+3,"speed"))
            {
             way.speed=kph_to_speed(parse_speed(id,k,v));
             recognised=1; break;
            }

          if(!strcmp(k+3,"weight"))
            {
             way.weight=tonnes_to_weight(parse_weight(id,k,v));
             recognised=1; break;
            }

          if(!strcmp(k+3,"height"))
            {
             way.height=metres_to_height(parse_length(id,k,v));
             recognised=1; break;
            }

          if(!strcmp(k+3,"width"))
            {
             way.width=metres_to_height(parse_length(id,k,v));
             recognised=1; break;
            }

          if(!strcmp(k+3,"length"))
            {
             way.length=metres_to_height(parse_length(id,k,v));
             recognised=1; break;
            }
         }

       if(!strcmp(k,"moped"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Moped;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'moped' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"motorcycle"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Motorcycle;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'motorcycle' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"motorcar"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Motorcar;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'motorcar' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"multilane"))
         {
          if(ISTRUE(v))
             way.props|=Properties_Multilane;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'multilane' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'n':
       if(!strcmp(k,"name"))
         {
          name=v;
          recognised=1; break;
         }

       break;

      case 'o':
       if(!strcmp(k,"oneway"))
         {
          if(ISTRUE(v))
             oneway=1;
          else if(!strcmp(v,"-1"))
             oneway=-1;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'oneway' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'p':
       if(!strcmp(k,"paved"))
         {
          if(ISTRUE(v))
             way.props|=Properties_Paved;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'paved' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       if(!strcmp(k,"psv"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_PSV;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'psv' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'r':
       if(!strcmp(k,"ref"))
         {
          ref=v;
          recognised=1; break;
         }

       if(!strcmp(k,"roundabout"))
         {
          if(ISTRUE(v))
             roundabout=1;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'roundabout' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 't':
       if(!strcmp(k,"tunnel"))
         {
          if(ISTRUE(v))
             way.props|=Properties_Tunnel;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'tunnel' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      case 'w':
       if(!strcmp(k,"wheelchair"))
         {
          if(ISTRUE(v))
             way.allow|=Transports_Wheelchair;
          else if(!ISFALSE(v))
             logerror("Way %"Pway_t" has an unrecognised tag 'wheelchair' = '%s' (after tagging rules); using 'no'.\n",logerror_way(id),v);
          recognised=1; break;
         }

       break;

      default:
       break;
      }

    if(!recognised)
       logerror("Way %"Pway_t" has an unrecognised tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),k,v);
   }

 /* Create the way */

 if(area && oneway)
   {
    logerror("Way %"Pway_t" is an area and oneway; ignoring area tag.\n",logerror_way(id));
    area=0;
   }

 if(cyclebothways && !oneway)
   {
    logerror("Way %"Pway_t" is cyclebothways but not oneway; ignoring cyclebothways tag.\n",logerror_way(id));
    cyclebothways=0;
   }

 if(roundabout && !oneway)
   {
    logerror("Way %"Pway_t" is roundabout but not oneway; adding oneway tag.\n",logerror_way(id));
    oneway=1;
   }

 if(!way.allow)
    return;

 if(cyclebothways)
    way.type|=Highway_CycleBothWays;

 if(oneway)
   {
    way.type|=Highway_OneWay;

    if(oneway==-1)
       for(i=0;i<way_nnodes/2;i++)
         {
          node_t temp;

          temp=way_nodes[i];
          way_nodes[i]=way_nodes[way_nnodes-i-1];
          way_nodes[way_nnodes-i-1]=temp;
         }
   }

 if(roundabout)
    way.type|=Highway_Roundabout;

 if(area)
   {
    way.type|=Highway_Area;

    if(way_nodes[0]!=way_nodes[way_nnodes-1])
       logerror("Way %"Pway_t" is an area but not closed.\n",logerror_way(id));
   }

 if(lanes)
   {
    if(oneway || (lanes/2)>1)
       way.props|=Properties_Multilane;

    if(oneway && lanes==1)
       way.props&=~Properties_Multilane;
   }

 if(ref && name)
   {
    refname=(char*)malloc(strlen(ref)+strlen(name)+4);
    sprintf(refname,"%s (%s)",name,ref);
   }
 else if(ref && !name)
    refname=ref;
 else if(!ref && name)
    refname=name;
 else /* if(!ref && !name) */
    refname="";

 AppendWayList(ways,id,&way,way_nodes,way_nnodes,refname);

 if(ref && name)
    free(refname);
}


/*++++++++++++++++++++++++++++++++++++++
  Process the tags associated with a relation.

  TagList *tags The list of relation tags.

  int64_t relation_id The id of the relation.

  int mode The mode of operation to take (create, modify, delete).
  ++++++++++++++++++++++++++++++++++++++*/

void ProcessRelationTags(TagList *tags,int64_t relation_id,int mode)
{
 transports_t routes=Transports_None;
 transports_t except=Transports_None;
 int relation_turn_restriction=0;
 TurnRestriction restriction=TurnRestrict_None;
 relation_t id;
 int i;

 /* Convert id */

 id=(relation_t)relation_id;
 logassert((int64_t)id==relation_id,"Relation ID too large (change relation_t to 64-bits?)"); /* check relation id can be stored in relation_t data type. */

 /* Delete */

 if(mode==MODE_DELETE || mode==MODE_MODIFY)
   {
    AppendRouteRelationList(relations,id,RELATION_DELETED,
                            relation_nodes,relation_nnodes,
                            relation_ways,relation_nways,
                            relation_relations,relation_nrelations);

    AppendTurnRelationList(relations,id,
                           relation_from,relation_to,relation_via,
                           restriction,RELATION_DELETED);
   }

 if(mode==MODE_DELETE)
    return;

 /* Sanity check */

 if(relation_nnodes==0 && relation_nways==0 && relation_nrelations==0)
   {
    logerror("Relation %"Prelation_t" has no nodes, ways or relations.\n",logerror_relation(id));
    return;
   }

 /* Parse the tags */

 for(i=0;i<tags->ntags;i++)
   {
    int recognised=0;
    char *k=tags->k[i];
    char *v=tags->v[i];

    switch(*k)
      {
      case 'b':
       if(!strcmp(k,"bicycleroute"))
         {
          if(ISTRUE(v))
             routes|=Transports_Bicycle;
          else if(!ISFALSE(v))
             logerror("Relation %"Prelation_t" has an unrecognised tag 'bicycleroute' = '%s' (after tagging rules); using 'no'.\n",logerror_relation(id),v);
          recognised=1; break;
         }

       break;

      case 'e':
       if(!strcmp(k,"except"))
         {
          for(i=1;i<Transport_Count;i++)
             if(strstr(v,TransportName(i)))
                except|=TRANSPORTS(i);

          if(except==Transports_None)
             logerror("Relation %"Prelation_t" has an unrecognised tag 'except' = '%s' (after tagging rules); ignoring it.\n",logerror_relation(id),v);

          recognised=1; break;
         }

       break;

      case 'f':
       if(!strcmp(k,"footroute"))
         {
          if(ISTRUE(v))
             routes|=Transports_Foot;
          else if(!ISFALSE(v))
             logerror("Relation %"Prelation_t" has an unrecognised tag 'footroute' = '%s' (after tagging rules); using 'no'.\n",logerror_relation(id),v);
          recognised=1; break;
         }

       break;

      case 'r':
       if(!strcmp(k,"restriction"))
         {
          if(!strcmp(v,"no_right_turn"   )) restriction=TurnRestrict_no_right_turn;
          if(!strcmp(v,"no_left_turn"    )) restriction=TurnRestrict_no_left_turn;
          if(!strcmp(v,"no_u_turn"       )) restriction=TurnRestrict_no_u_turn;
          if(!strcmp(v,"no_straight_on"  )) restriction=TurnRestrict_no_straight_on;
          if(!strcmp(v,"only_right_turn" )) restriction=TurnRestrict_only_right_turn;
          if(!strcmp(v,"only_left_turn"  )) restriction=TurnRestrict_only_left_turn;
          if(!strcmp(v,"only_straight_on")) restriction=TurnRestrict_only_straight_on;

          if(restriction==TurnRestrict_None)
             logerror("Relation %"Prelation_t" has an unrecognised tag 'restriction' = '%s' (after tagging rules); ignoring it.\n",logerror_relation(id),v);

          recognised=1; break;
         }

       break;

      case 't':
       if(!strcmp(k,"type"))
         {
          if(!strcmp(v,"restriction"))
             relation_turn_restriction=1;

          /* Don't log an error for relations of types that we don't handle - there are so many */
          recognised=1; break;
         }

       break;

      default:
       break;
      }

    if(!recognised)
       logerror("Relation %"Prelation_t" has an unrecognised tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_relation(id),k,v);
   }

 /* Create the route relation (must store all relations that have ways or
    relations even if they are not routes because they might be referenced by
    other relations that are routes) */

 if((relation_nways || relation_nrelations) && !relation_turn_restriction)
    AppendRouteRelationList(relations,id,routes,
                            relation_nodes,relation_nnodes,
                            relation_ways,relation_nways,
                            relation_relations,relation_nrelations);

 /* Create the turn restriction relation. */

 if(relation_turn_restriction && restriction!=TurnRestrict_None)
   {
    if(relation_from==NO_WAY_ID)
      {
       /* Extra logerror information since relation isn't stored */
       if(relation_to!=NO_WAY_ID) logerror_way(relation_to);
       if(relation_via!=NO_NODE_ID) logerror_node(relation_via);
       logerror("Turn Relation %"Prelation_t" has no 'from' way.\n",logerror_relation(id));
      }
    if(relation_to==NO_WAY_ID)
      {
       /* Extra logerror information since relation isn't stored */
       if(relation_via!=NO_NODE_ID) logerror_node(relation_via);
       if(relation_from!=NO_WAY_ID) logerror_way(relation_from);
       logerror("Turn Relation %"Prelation_t" has no 'to' way.\n",logerror_relation(id));
      }
    if(relation_via==NO_NODE_ID)
      {
       /* Extra logerror information since relation isn't stored */
       if(relation_to!=NO_WAY_ID) logerror_way(relation_to);
       if(relation_from!=NO_WAY_ID) logerror_way(relation_from);
       logerror("Turn Relation %"Prelation_t" has no 'via' node.\n",logerror_relation(id));
      }

    if(relation_from_count>1)
      {
       if(relation_to!=NO_WAY_ID) logerror_way(relation_to);
       if(relation_via!=NO_NODE_ID) logerror_node(relation_via);
       if(relation_from!=NO_WAY_ID) logerror_way(relation_from);
       logerror("Turn Relation %"Prelation_t" has more than one 'from' Way (used first one).\n",logerror_relation(relation_id));
      }
    if(relation_to_count>1)
      {
       if(relation_to!=NO_WAY_ID) logerror_way(relation_to);
       if(relation_via!=NO_NODE_ID) logerror_node(relation_via);
       if(relation_from!=NO_WAY_ID) logerror_way(relation_from);
       logerror("Turn Relation %"Prelation_t" has more than one 'to' Way (used first one).\n",logerror_relation(relation_id));
      }
    if(relation_via_count>1)
      {
       if(relation_to!=NO_WAY_ID) logerror_way(relation_to);
       if(relation_via!=NO_NODE_ID) logerror_node(relation_via);
       if(relation_from!=NO_WAY_ID) logerror_way(relation_from);
       logerror("Turn Relation %"Prelation_t" has more than one 'via' Node (used first one).\n",logerror_relation(relation_id));
      }

    if(relation_from!=NO_WAY_ID && relation_to!=NO_WAY_ID && relation_via!=NO_NODE_ID)
       AppendTurnRelationList(relations,id,
                              relation_from,relation_to,relation_via,
                              restriction,except);
   }
}


/*++++++++++++++++++++++++++++++++++++++
  Convert a string containing a speed into a double precision.

  double parse_speed Returns the speed in km/h if it can be parsed.

  way_t id The way being processed.

  const char *k The tag key.

  const char *v The tag value.
  ++++++++++++++++++++++++++++++++++++++*/

static double parse_speed(way_t id,const char *k,const char *v)
{
 char *ev;
 double value=strtod(v,&ev);

 if(v==ev)
    logerror("Way %"Pway_t" has an unrecognised tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),k,v);
 else
   {
    while(isspace(*ev)) ev++;

    if(*ev==0)
       return(value);

    if(!strcmp(ev,"km/h") || !strcmp(ev,"kph") || !strcmp(ev,"kmph"))
       return(value);

    if(!strcmp(ev,"mph"))
       return(1.609*value);

    if(!strcmp(ev,"knots"))
       return(1.852*value);

    logerror("Way %"Pway_t" has an un-parseable tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),k,v);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Convert a string containing a weight into a double precision.

  double parse_weight Returns the weight in tonnes if it can be parsed.

  way_t id The way being processed.

  const char *k The tag key.

  const char *v The tag value.
  ++++++++++++++++++++++++++++++++++++++*/

static double parse_weight(way_t id,const char *k,const char *v)
{
 char *ev;
 double value=strtod(v,&ev);

 if(v==ev)
    logerror("Way %"Pway_t" has an unrecognised tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),k,v);
 else
   {
    while(isspace(*ev)) ev++;

    if(*ev==0)
       return(value);

    if(!strcmp(ev,"kg"))
       return(value/1000.0);

    if(!strcmp(ev,"T") || !strcmp(ev,"t") ||
       !strcmp(ev,"ton") || !strcmp(ev,"tons") ||
       !strcmp(ev,"tonne") || !strcmp(ev,"tonnes"))
       return(value);

    logerror("Way %"Pway_t" has an un-parseable tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),k,v);
   }

 return(0);
}


/*++++++++++++++++++++++++++++++++++++++
  Convert a string containing a length into a double precision.

  double parse_length Returns the length in metres if it can be parsed.

  way_t id The way being processed.

  const char *k The tag key.

  const char *v The tag value.
  ++++++++++++++++++++++++++++++++++++++*/

static double parse_length(way_t id,const char *k,const char *v)
{
 char *ev;
 double value=strtod(v,&ev);

 if(v==ev)
    logerror("Way %"Pway_t" has an unrecognised tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),k,v);
 else
   {
    int en=0;
    int feet=0,inches=0;

    while(isspace(*ev)) ev++;

    if(*ev==0)
       return(value);

    if(!strcmp(ev,"m") || !strcmp(ev,"metre") || !strcmp(ev,"metres") || !strcmp(ev,"meter") || !strcmp(ev,"meters"))
       return(value);

    if(!strcmp(ev,"'"))
       return(value*0.254);

    if(!strcmp(ev,"???"))
       return(value*0.254);

    if(!strcmp(ev,"ft") || !strcmp(ev,"feet"))
       return(value*0.254);

    if(sscanf(v,"%d' %d\"%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d'%d\"%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d'-%d\"%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d??? %d???%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d???%d???%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d???-%d???%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d - %d%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d ft %d in%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    if(sscanf(v,"%d feet %d inches%n",&feet,&inches,&en)==2 && en && !v[en])
       return((feet+(double)inches/12.0)*0.254);

    logerror("Way %"Pway_t" has an un-parseable tag '%s' = '%s' (after tagging rules); ignoring it.\n",logerror_way(id),k,v);
   }

 return(0);
}
