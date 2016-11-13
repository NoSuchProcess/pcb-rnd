/*
 * read hyperlynx files
 * Copyright 2012 Koen De Vleeschauwer.
 *
 * This file is part of pcb-rnd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "parser.h"
#include "hyp_l.h"
#include "hyp_y.h"
#include "error.h"

int hyp_debug;

/* called by pcb-rnd to load hyperlynx file */
int hyp_parse(const char *fname, int debug)
{
  int retval;

  /* set debug levels */
  hyyset_debug(debug > 2); /* switch flex logging on */
  hyydebug = (debug > 1);  /* switch bison logging on */
  hyp_debug = (debug > 0); /* switch hyperlynx logging on */

  /* parse hyperlynx file */
  hyyin =  fopen(fname, "r");
  if (hyyin == NULL) return (1);
  retval = hyyparse();
  fclose(hyyin);

  return(retval);
}
 
/* print error message */
void hyp_error(const char *msg)
{
  Message(PCB_MSG_DEBUG, "line %d: %s at '%s'\n", hyylineno, msg, hyytext);
}

/* exec_* routines are called by parser to interpret hyperlynx file */ 
pcb_bool exec_board_file(parse_param *h){return(0);}
pcb_bool exec_version(parse_param *h){return(0);}
pcb_bool exec_data_mode(parse_param *h){return(0);}
pcb_bool exec_units(parse_param *h){return(0);}
pcb_bool exec_plane_sep(parse_param *h){return(0);}
pcb_bool exec_perimeter_segment(parse_param *h){return(0);}
pcb_bool exec_perimeter_arc(parse_param *h){return(0);}
pcb_bool exec_board_attribute(parse_param *h){return(0);}

pcb_bool exec_options(parse_param *h){return(0);}
pcb_bool exec_signal(parse_param *h){return(0);}
pcb_bool exec_dielectric(parse_param *h){return(0);}
pcb_bool exec_plane(parse_param *h){return(0);}

pcb_bool exec_devices(parse_param *h){return(0);}

pcb_bool exec_supplies(parse_param *h){return(0);}

pcb_bool exec_padstack_element(parse_param *h){return(0);}
pcb_bool exec_padstack_end(parse_param *h){return(0);}

pcb_bool exec_net(parse_param *h){return(0);}
pcb_bool exec_net_plane_separation(parse_param *h){return(0);}
pcb_bool exec_net_attribute(parse_param *h){return(0);}
pcb_bool exec_seg(parse_param *h){return(0);}
pcb_bool exec_arc(parse_param *h){return(0);}
pcb_bool exec_via(parse_param *h){return(0);}
pcb_bool exec_via_v1(parse_param *h){return(0);}
pcb_bool exec_pin(parse_param *h){return(0);}
pcb_bool exec_pad(parse_param *h){return(0);}
pcb_bool exec_useg(parse_param *h){return(0);}

pcb_bool exec_polygon_begin(parse_param *h){return(0);}
pcb_bool exec_polygon_end(parse_param *h){return(0);}
pcb_bool exec_polyvoid_begin(parse_param *h){return(0);}
pcb_bool exec_polyvoid_end(parse_param *h){return(0);}
pcb_bool exec_polyline_begin(parse_param *h){return(0);}
pcb_bool exec_polyline_end(parse_param *h){return(0);}
pcb_bool exec_line(parse_param *h){return(0);}
pcb_bool exec_curve(parse_param *h){return(0);}

pcb_bool exec_net_class(parse_param *h){return(0);}
pcb_bool exec_net_class_element(parse_param *h){return(0);}
pcb_bool exec_net_class_attribute(parse_param *h){return(0);}

pcb_bool exec_end(parse_param *h){return(0);}
pcb_bool exec_key(parse_param *h){return(0);}

/* not truncated */
