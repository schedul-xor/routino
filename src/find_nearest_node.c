
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "version.h"

#include "types.h"
#include "nodes.h"
#include "segments.h"
#include "ways.h"
#include "relations.h"

#include "files.h"
#include "logging.h"
#include "functions.h"
#include "fakes.h"
#include "translations.h"
#include "profiles.h"


/*+ The maximum distance from the specified point to search for a node or segment (in km). +*/
#define MAXSEARCH  1

/* Global variables */

/*+ The option not to print any progress information. +*/
int option_quiet=0;

/*+ The option to calculate the quickest route insted of the shortest. +*/
extern int option_quickest;

/*+ The options to select the format of the file output. +*/
extern int option_file_html,option_file_gpx_track,option_file_gpx_route,option_file_text,option_file_text_all,option_file_stdout;
int option_file_none=0;

int main(int argc,char** argv)
{
 Nodes       *OSMNodes;
 Segments    *OSMSegments;
 Ways        *OSMWays;
 Relations   *OSMRelations;
 Results     *results[NWAYPOINTS+1]={NULL};
 int          point_used[NWAYPOINTS+1]={0};
 double       point_lon[NWAYPOINTS+1],point_lat[NWAYPOINTS+1];
 index_t      point_node[NWAYPOINTS+1]={NO_NODE};
 double       heading=-999;
 int          help_profile=0,help_profile_xml=0,help_profile_json=0,help_profile_pl=0;
 char        *dirname=NULL,*prefix=NULL;
 char        *profiles=NULL,*profilename=NULL;
 char        *translations=NULL,*language=NULL;
 int          exactnodes=0,reverse=0,loop=0;
 Transport    transport=Transport_None;
 Profile     *profile=NULL;
 Translation *translation=NULL;
 index_t      start_node,finish_node=NO_NODE;
 index_t      join_segment=NO_SEGMENT;
 int          arg,nresults=0;
 waypoint_t   start_waypoint,finish_waypoint=NO_WAYPOINT;
 waypoint_t   first_waypoint=NWAYPOINTS,last_waypoint=1,waypoint;
 int          inc_dec_waypoint=1;

 printf("Find nearest node\n");

 if(argc<2)
    print_usage(0,NULL,NULL);

 for(arg=1;arg<argc;arg++)
   {
    if(!strcmp(argv[arg],"--version"))
       print_usage(-1,NULL,NULL);
    else if(!strcmp(argv[arg],"--help"))
       print_usage(1,NULL,NULL);
    else if(!strcmp(argv[arg],"--help-profile"))
       help_profile=1;
    else if(!strcmp(argv[arg],"--help-profile-xml"))
       help_profile_xml=1;
    else if(!strcmp(argv[arg],"--help-profile-json"))
       help_profile_json=1;
    else if(!strcmp(argv[arg],"--help-profile-perl"))
       help_profile_pl=1;
    else if(!strncmp(argv[arg],"--dir=",6))
       dirname=&argv[arg][6];
    else if(!strncmp(argv[arg],"--prefix=",9))
       prefix=&argv[arg][9];
    else if(!strncmp(argv[arg],"--profiles=",11))
       profiles=&argv[arg][11];
    else if(!strncmp(argv[arg],"--translations=",15))
       translations=&argv[arg][15];
    else if(!strcmp(argv[arg],"--exact-nodes-only"))
       exactnodes=1;
    else if(!strncmp(argv[arg],"--reverse",9))
      {
       if(argv[arg][9]=='=')
          reverse=atoi(&argv[arg][10]);
       else
          reverse=1;
      }
    else if(!strncmp(argv[arg],"--loop",6))
      {
       if(argv[arg][6]=='=')
          loop=atoi(&argv[arg][7]);
       else
          loop=1;
      }
    else if(!strcmp(argv[arg],"--quiet"))
       option_quiet=1;
    else if(!strcmp(argv[arg],"--loggable"))
       option_loggable=1;
    else if(!strcmp(argv[arg],"--logtime"))
       option_logtime=2;
    else if(!strcmp(argv[arg],"--logmemory"))
       option_logmemory=1;
    else if(!strcmp(argv[arg],"--output-html"))
       option_file_html=1;
    else if(!strcmp(argv[arg],"--output-gpx-track"))
       option_file_gpx_track=1;
    else if(!strcmp(argv[arg],"--output-gpx-route"))
       option_file_gpx_route=1;
    else if(!strcmp(argv[arg],"--output-text"))
       option_file_text=1;
    else if(!strcmp(argv[arg],"--output-text-all"))
       option_file_text_all=1;
    else if(!strcmp(argv[arg],"--output-none"))
       option_file_none=1;
    else if(!strcmp(argv[arg],"--output-stdout"))
      { option_file_stdout=1; option_quiet=1; }
    else if(!strncmp(argv[arg],"--profile=",10))
       profilename=&argv[arg][10];
    else if(!strncmp(argv[arg],"--language=",11))
       language=&argv[arg][11];
    else if(!strcmp(argv[arg],"--shortest"))
       option_quickest=0;
    else if(!strcmp(argv[arg],"--quickest"))
       option_quickest=1;
    else if(!strncmp(argv[arg],"--lon",5) && isdigit(argv[arg][5]))
      {
       int point;
       char *p=&argv[arg][6];

       while(isdigit(*p)) p++;
       if(*p++!='=')
          print_usage(0,argv[arg],NULL);

       point=atoi(&argv[arg][5]);
       if(point>NWAYPOINTS || point_used[point]&1)
          print_usage(0,argv[arg],NULL);

       point_lon[point]=degrees_to_radians(atof(p));
       point_used[point]+=1;

       if(point<first_waypoint)
          first_waypoint=point;
       if(point>last_waypoint)
          last_waypoint=point;
      }
    else if(!strncmp(argv[arg],"--lat",5) && isdigit(argv[arg][5]))
      {
       int point;
       char *p=&argv[arg][6];

       while(isdigit(*p)) p++;
       if(*p++!='=')
          print_usage(0,argv[arg],NULL);

       point=atoi(&argv[arg][5]);
       if(point>NWAYPOINTS || point_used[point]&2)
          print_usage(0,argv[arg],NULL);

       point_lat[point]=degrees_to_radians(atof(p));
       point_used[point]+=2;

       if(point<first_waypoint)
          first_waypoint=point;
       if(point>last_waypoint)
          last_waypoint=point;
      }
    else if(!strncmp(argv[arg],"--heading=",10))
      {
       double h=atof(&argv[arg][10]);

       if(h>=-360 && h<=360)
         {
          heading=h;

          if(heading<0) heading+=360;
         }
      }
    else if(!strncmp(argv[arg],"--transport=",12))
      {
       transport=TransportType(&argv[arg][12]);

       if(transport==Transport_None)
          print_usage(0,argv[arg],NULL);
      }
    else
       continue;

    argv[arg]=NULL;
   }


 if(option_file_stdout && (option_file_html+option_file_gpx_track+option_file_gpx_route+option_file_text+option_file_text_all)!=1)
   {
    fprintf(stderr,"Error: The '--output-stdout' option requires exactly one other output option (but not '--output-none').\n");
    exit(EXIT_FAILURE);
   }

 if(option_file_html==0 && option_file_gpx_track==0 && option_file_gpx_route==0 && option_file_text==0 && option_file_text_all==0 && option_file_none==0)
    option_file_html=option_file_gpx_track=option_file_gpx_route=option_file_text=option_file_text_all=1;

 /* Load in the selected profiles */

 if(transport==Transport_None)
    transport=Transport_Motorcar;
 
 if(profiles)
   {
    if(!ExistsFile(profiles))
      {
       fprintf(stderr,"Error: The '--profiles' option specifies a file '%s' that does not exist.\n",profiles);
       exit(EXIT_FAILURE);
      }
   }
 else
   {
    profiles=FileName(dirname,prefix,"profiles.xml");

    if(!ExistsFile(profiles))
      {
       char *defaultprofiles=FileName(ROUTINO_DATADIR,NULL,"profiles.xml");

       if(!ExistsFile(defaultprofiles))
         {
          fprintf(stderr,"Error: The '--profiles' option was not used and the files '%s' and '%s' do not exist.\n",profiles,defaultprofiles);
          exit(EXIT_FAILURE);
         }

       free(profiles);
       profiles=defaultprofiles;
      }
   }

 if(!profilename)
    profilename=(char*)TransportName(transport);

 if(ParseXMLProfiles(profiles,profilename,(help_profile_xml|help_profile_json|help_profile_pl)))
   {
    fprintf(stderr,"Error: Cannot read the profiles in the file '%s'.\n",profiles);
    exit(EXIT_FAILURE);
   }

 profile=GetProfile(profilename);

 if(!profile)
   {
    fprintf(stderr,"Error: Cannot find a profile called '%s' in the file '%s'.\n",profilename,profiles);

    profile=(Profile*)calloc(1,sizeof(Profile));
    profile->transport=transport;
   }

 /* Parse the other command line arguments that modify the profile */

 for(arg=1;arg<argc;arg++)
   {
    if(!argv[arg])
       continue;
    else if(!strncmp(argv[arg],"--highway-",10))
      {
       Highway highway;
       char *equal=strchr(argv[arg],'=');
       char *string;
       double p;

       if(!equal)
           print_usage(0,argv[arg],NULL);

       string=strcpy((char*)malloc(strlen(argv[arg])),argv[arg]+10);
       string[equal-argv[arg]-10]=0;

       highway=HighwayType(string);

       if(highway==Highway_None)
          print_usage(0,argv[arg],NULL);

       p=atof(equal+1);

       if(p<0 || p>100)
          print_usage(0,argv[arg],NULL);

       profile->highway[highway]=(score_t)(p/100);

       free(string);
      }
    else if(!strncmp(argv[arg],"--speed-",8))
      {
       Highway highway;
       char *equal=strchr(argv[arg],'=');
       char *string;
       double s;

       if(!equal)
          print_usage(0,argv[arg],NULL);

       string=strcpy((char*)malloc(strlen(argv[arg])),argv[arg]+8);
       string[equal-argv[arg]-8]=0;

       highway=HighwayType(string);

       if(highway==Highway_None)
          print_usage(0,argv[arg],NULL);

       s=atof(equal+1);

       if(s<0)
          print_usage(0,argv[arg],NULL);

       profile->speed[highway]=kph_to_speed(s);

       free(string);
      }
    else if(!strncmp(argv[arg],"--property-",11))
      {
       Property property;
       char *equal=strchr(argv[arg],'=');
       char *string;
       double p;

       if(!equal)
          print_usage(0,argv[arg],NULL);

       string=strcpy((char*)malloc(strlen(argv[arg])),argv[arg]+11);
       string[equal-argv[arg]-11]=0;

       property=PropertyType(string);

       if(property==Property_None)
          print_usage(0,argv[arg],NULL);

       p=atof(equal+1);

       if(p<0 || p>100)
          print_usage(0,argv[arg],NULL);

       profile->props[property]=(score_t)(p/100);

       free(string);
      }
    else if(!strncmp(argv[arg],"--oneway=",9))
       profile->oneway=!!atoi(&argv[arg][9]);
    else if(!strncmp(argv[arg],"--turns=",8))
       profile->turns=!!atoi(&argv[arg][8]);
    else if(!strncmp(argv[arg],"--weight=",9))
       profile->weight=tonnes_to_weight(atof(&argv[arg][9]));
    else if(!strncmp(argv[arg],"--height=",9))
       profile->height=metres_to_height(atof(&argv[arg][9]));
    else if(!strncmp(argv[arg],"--width=",8))
       profile->width=metres_to_width(atof(&argv[arg][8]));
    else if(!strncmp(argv[arg],"--length=",9))
       profile->length=metres_to_length(atof(&argv[arg][9]));
    else
       print_usage(0,argv[arg],NULL);
   }

 /* Print one of the profiles if requested */

 if(help_profile)
   {
    PrintProfile(profile);

    exit(EXIT_SUCCESS);
   }
 else if(help_profile_xml)
   {
    PrintProfilesXML();

    exit(EXIT_SUCCESS);
   }
 else if(help_profile_json)
   {
    PrintProfilesJSON();

    exit(EXIT_SUCCESS);
   }
 else if(help_profile_pl)
   {
    PrintProfilesPerl();

    exit(EXIT_SUCCESS);
   }

 /* Load in the selected translation */

 if(option_file_html || option_file_gpx_route || option_file_gpx_track || option_file_text || option_file_text_all)
   {
    if(translations)
      {
       if(!ExistsFile(translations))
         {
          fprintf(stderr,"Error: The '--translations' option specifies a file '%s' that does not exist.\n",translations);
          exit(EXIT_FAILURE);
         }
      }
    else
      {
       translations=FileName(dirname,prefix,"translations.xml");

       if(!ExistsFile(translations))
         {
          char *defaulttranslations=FileName(ROUTINO_DATADIR,NULL,"translations.xml");

          if(!ExistsFile(defaulttranslations))
            {
             fprintf(stderr,"Error: The '--translations' option was not used and the files '%s' and '%s' do not exist.\n",translations,defaulttranslations);
             exit(EXIT_FAILURE);
            }

          free(translations);
          translations=defaulttranslations;
         }
      }

    if(ParseXMLTranslations(translations,language,0))
      {
       fprintf(stderr,"Error: Cannot read the translations in the file '%s'.\n",translations);
       exit(EXIT_FAILURE);
      }

    if(language)
      {
       translation=GetTranslation(language);

       if(!translation)
         {
          fprintf(stderr,"Error: Cannot find a translation called '%s' in the file '%s'.\n",language,translations);
          exit(EXIT_FAILURE);
         }
      }
    else
      {
       translation=GetTranslation("");

       if(!translation)
         {
          fprintf(stderr,"Error: No translations in '%s'.\n",translations);
          exit(EXIT_FAILURE);
         }
      }
   }

 /* Check the waypoints are valid */

 for(waypoint=1;waypoint<=NWAYPOINTS;waypoint++)
    if(point_used[waypoint]==1 || point_used[waypoint]==2)
       print_usage(0,NULL,"All waypoints must have latitude and longitude.");

 /* Load in the data - Note: No error checking because Load*List() will call exit() in case of an error. */

 if(!option_quiet)
    printf_first("Loading Files:");

 OSMNodes=LoadNodeList(FileName(dirname,prefix,"nodes.mem"));

 OSMSegments=LoadSegmentList(FileName(dirname,prefix,"segments.mem"));

 OSMWays=LoadWayList(FileName(dirname,prefix,"ways.mem"));

 OSMRelations=LoadRelationList(FileName(dirname,prefix,"relations.mem"));

 if(!option_quiet)
    printf_last("Loaded Files: nodes, segments, ways & relations");

 /* Check the profile is valid for use with this database */

 if(UpdateProfile(profile,OSMWays))
   {
    fprintf(stderr,"Error: Profile is invalid or not compatible with database.\n");
    exit(EXIT_FAILURE);
   }

 /* Find all waypoints */

 for(waypoint=first_waypoint;waypoint<=last_waypoint;waypoint++)
   {
    distance_t distmax=km_to_distance(MAXSEARCH);
    distance_t distmin;
    index_t segment=NO_SEGMENT;
    index_t node1,node2,node=NO_NODE;

    if(point_used[waypoint]!=3)
       continue;

    /* Find the closest point */

    if(!option_quiet)
       printf_first("Finding Closest Point: Waypoint %d",waypoint);

    if(exactnodes)
       node=FindClosestNode(OSMNodes,OSMSegments,OSMWays,point_lat[waypoint],point_lon[waypoint],distmax,profile,&distmin);
    else
      {
       distance_t dist1,dist2;

       segment=FindClosestSegment(OSMNodes,OSMSegments,OSMWays,point_lat[waypoint],point_lon[waypoint],distmax,profile,&distmin,&node1,&node2,&dist1,&dist2);

       if(segment!=NO_SEGMENT)
          node=CreateFakes(OSMNodes,OSMSegments,waypoint,LookupSegment(OSMSegments,segment,1),node1,node2,dist1,dist2);
      }

    if(!option_quiet)
       printf_last("Found Closest Point: Waypoint %d",waypoint);

    if(node==NO_NODE)
      {
       fprintf(stderr,"Error: Cannot find node close to specified point %d.\n",waypoint);
       exit(EXIT_FAILURE);
      }

    if(!option_quiet)
      {
       double lat,lon;

       if(IsFakeNode(node))
          GetFakeLatLong(node,&lat,&lon);
       else
          GetLatLong(OSMNodes,node,NULL,&lat,&lon);

       if(IsFakeNode(node))
          printf("Waypoint %d is segment %"Pindex_t" (node %"Pindex_t" -> %"Pindex_t"): %3.6f %4.6f = %2.3f km\n",waypoint,segment,node1,node2,
                 radians_to_degrees(lon),radians_to_degrees(lat),distance_to_km(distmin));
       else
          printf("Waypoint %d is node %"Pindex_t": %3.6f %4.6f = %2.3f km\n",waypoint,node,
                 radians_to_degrees(lon),radians_to_degrees(lat),distance_to_km(distmin));
      }

    point_node[waypoint]=node;
   }

 return 0;
}
