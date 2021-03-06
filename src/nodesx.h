/***************************************
 A header file for the extended nodes.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2008-2015, 2019 Andrew M. Bishop

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


#ifndef NODESX_H
#define NODESX_H    /*+ To stop multiple inclusions. +*/

#include <stdint.h>

#include "types.h"
#include "nodes.h"

#include "typesx.h"

#include "cache.h"
#include "files.h"


/* Data structures */


/*+ An extended structure used for processing. +*/
struct _NodeX
{
 node_t       id;               /*+ The node identifier; initially the OSM value, later the Node index, finally the first segment. +*/

 latlong_t    latitude;         /*+ The node latitude. +*/
 latlong_t    longitude;        /*+ The node longitude. +*/

 transports_t allow;            /*+ The types of transport that are allowed through the node. +*/

 nodeflags_t  flags;            /*+ Flags containing extra information (e.g. super-node, turn restriction). +*/
};

/*+ A structure containing a set of nodes (memory format). +*/
struct _NodesX
{
 char     *filename;            /*+ The name of the intermediate file (for the NodesX). +*/
 char     *filename_tmp;        /*+ The name of the temporary file (for the NodesX). +*/

 int       fd;                  /*+ The file descriptor of the open file (for the NodesX). +*/

 index_t   number;              /*+ The number of extended nodes still being considered. +*/
 index_t   knumber;             /*+ The number of extended nodes kept for next time. +*/

#if !SLIM

 NodeX    *data;                /*+ The extended node data (when mapped into memory). +*/

#else

 NodeX     cached[3];           /*+ Three cached extended nodes read from the file in slim mode. +*/
 index_t   incache[3];          /*+ The indexes of the cached extended nodes. +*/

 NodeXCache *cache;             /*+ A RAM cache of extended nodes read from the file. +*/

#endif

 char     *ifilename_tmp;       /*+ The name of the temporary file (for the NodesX ID index). +*/

 int       ifd;                 /*+ The file descriptor of the temporary file (for the NodesX ID index). +*/

 node_t   *idata;               /*+ The extended node IDs (sorted by ID). +*/

 index_t  *pdata;               /*+ The node indexes after pruning. +*/

 index_t  *gdata;               /*+ The final node indexes (sorted geographically). +*/

 BitMask  *super;               /*+ A bit-mask marker for super nodes (same order as sorted nodes). +*/

 index_t   latbins;             /*+ The number of bins containing latitude. +*/
 index_t   lonbins;             /*+ The number of bins containing longitude. +*/

 ll_bin_t  latzero;             /*+ The bin number of the furthest south bin. +*/
 ll_bin_t  lonzero;             /*+ The bin number of the furthest west bin. +*/
};


/* Functions in nodesx.c */

NodesX *NewNodeList(int append,int readonly);
void FreeNodeList(NodesX *nodesx,int keep);

void AppendNodeList(NodesX *nodesx,node_t id,double latitude,double longitude,transports_t allow,nodeflags_t flags);
void FinishNodeList(NodesX *nodesx);

index_t IndexNodeX(NodesX *nodesx,node_t id);

void SortNodeList(NodesX *nodesx);

void RemoveNonHighwayNodes(NodesX *nodesx,WaysX *waysx,int keep);

void RemovePrunedNodes(NodesX *nodesx,SegmentsX *segmentsx);

void SortNodeListGeographically(NodesX *nodesx);

void SaveNodeList(NodesX *nodesx,const char *filename,SegmentsX *segmentsx);


/* Macros and inline functions */

#if !SLIM

#define LookupNodeX(nodesx,index,position)      &(nodesx)->data[index]
  
#define PutBackNodeX(nodesx,nodex)              while(0) { /* nop */ }

#else

/* Prototypes */

static inline NodeX *LookupNodeX(NodesX *nodesx,index_t index,int position);

static inline void PutBackNodeX(NodesX *nodesx,NodeX *nodex);

CACHE_NEWCACHE_PROTO(NodeX)
CACHE_DELETECACHE_PROTO(NodeX)
CACHE_FETCHCACHE_PROTO(NodeX)
CACHE_REPLACECACHE_PROTO(NodeX)
CACHE_INVALIDATECACHE_PROTO(NodeX)

/* Data type */

CACHE_STRUCTURE(NodeX)

/* Inline functions */

CACHE_NEWCACHE(NodeX)
CACHE_DELETECACHE(NodeX)
CACHE_FETCHCACHE(NodeX)
CACHE_REPLACECACHE(NodeX)
CACHE_INVALIDATECACHE(NodeX)


/*++++++++++++++++++++++++++++++++++++++
  Lookup a particular extended node with the specified id from the file on disk.

  NodeX *LookupNodeX Returns a pointer to a cached copy of the extended node.

  NodesX *nodesx The set of nodes to use.

  index_t index The node index to look for.

  int position The position in the cache to use.
  ++++++++++++++++++++++++++++++++++++++*/

static inline NodeX *LookupNodeX(NodesX *nodesx,index_t index,int position)
{
 nodesx->cached[position-1]=*FetchCachedNodeX(nodesx->cache,index,nodesx->fd,0);

 nodesx->incache[position-1]=index;

 return(&nodesx->cached[position-1]);
}


/*++++++++++++++++++++++++++++++++++++++
  Put back an extended node's data into the file on disk.

  NodesX *nodesx The set of nodes to modify.

  NodeX *nodex The extended node to be put back.
  ++++++++++++++++++++++++++++++++++++++*/

static inline void PutBackNodeX(NodesX *nodesx,NodeX *nodex)
{
 int position1=nodex-&nodesx->cached[0];

 ReplaceCachedNodeX(nodesx->cache,nodex,nodesx->incache[position1],nodesx->fd,0);
}

#endif /* SLIM */


#endif /* NODESX_H */
