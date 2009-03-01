/***************************************
 $Header: /home/amb/CVS/routino/src/profiles.h,v 1.4 2009-03-01 17:37:14 amb Exp $

 A header file for the profiles.
 ******************/ /******************
 Written by Andrew M. Bishop

 This file Copyright 2009 Andrew M. Bishop
 It may be distributed under the GNU Public License, version 2, or
 any higher version.  See section COPYING of the GNU Public license
 for conditions under which this file may be redistributed.
 ***************************************/


#ifndef PROFILES_H
#define PROFILES_H    /*+ To stop multiple inclusions. +*/

#include "types.h"


/* Data structures */

/*+ A data structure to hold a transport type profile. +*/
typedef struct _Profile
{
 Transport transport;           /*+ The type of transport. +*/

 Allowed   allow;               /*+ The type of transport expressed as what must be allowed on a way. +*/

 int       highways[Way_Unknown]; /*+ A flag to indicate if the transport is allowed on the highway. +*/

 speed_t   speed[Way_Unknown];  /*+ The maximum speed on each type of highway. +*/

 int       oneway;              /*+ A flag to indicate if one-way restrictions apply. +*/

 weight_t  weight;              /*+ The minimum weight of the route. +*/

 height_t  height;              /*+ The minimum height of vehicles on the route. +*/
 width_t   width;               /*+ The minimum width of vehicles on the route. +*/
 length_t  length;              /*+ The minimum length of vehicles on the route. +*/
}
 Profile;


/* Functions */

Profile *GetProfile(Transport transport);

void PrintProfile(const Profile *profile);

void PrintProfilesJS(void);

#endif /* PROFILES_H */
