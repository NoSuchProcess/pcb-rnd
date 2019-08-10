li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = HC51U Crystals
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 40.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -40.0mil
        -40.0mil
        40.0mil
        -40.0mil
        40.0mil
        40.0mil
        -40.0mil
        40.0mil
       }
       ha:layer_mask {
        copper = 1
        top = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -40.0mil
        -40.0mil
        40.0mil
        -40.0mil
        40.0mil
        40.0mil
        -40.0mil
        40.0mil
       }
       ha:layer_mask {
        bottom = 1
        copper = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -40.0mil
        -40.0mil
        40.0mil
        -40.0mil
        40.0mil
        40.0mil
        -40.0mil
        40.0mil
       }
       ha:layer_mask {
        copper = 1
        intern = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -43.0mil
        -43.0mil
        43.0mil
        -43.0mil
        43.0mil
        43.0mil
        -43.0mil
        43.0mil
       }
       ha:layer_mask {
        top = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -43.0mil
        -43.0mil
        43.0mil
        -43.0mil
        43.0mil
        43.0mil
        -43.0mil
        43.0mil
       }
       ha:layer_mask {
        bottom = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 1
    }
    ha:ps_proto_v6.1 {
     htop = 0
     hdia = 40.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 80.0mil
       }
       ha:layer_mask {
        copper = 1
        top = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 80.0mil
       }
       ha:layer_mask {
        bottom = 1
        copper = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 80.0mil
       }
       ha:layer_mask {
        copper = 1
        intern = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 86.0mil
       }
       ha:layer_mask {
        top = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 86.0mil
       }
       ha:layer_mask {
        bottom = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 1
    }
   }
   li:objects {
    ha:padstack_ref.7 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 3.71348mm
     rot = 0.000000
     y = 4.72948mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.8 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 1
     xmirror = 0
     x = 631.2mil
     rot = 0.000000
     y = 4.72948mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
   }
   li:layers {
    ha:top-silk {
     lid = 0
     ha:type {
      silk = 1
      top = 1
     }
     li:objects {
      ha:line.9 {
       clearance = 0.0
       y2 = 0.25908mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 4.72948mm
       x2 = 591.2mil
       ha:flags {
       }
       y1 = 0.25908mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 9.19988mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 591.2mil
       x2 = 4.72948mm
       ha:flags {
       }
       y1 = 9.19988mm
      }
      ha:arc.12 {
       astart = 90
       thickness = 20.0mil
       width = 176.0mil
       height = 176.0mil
       ha:attributes {
       }
       x = 591.2mil
       y = 4.72948mm
       adelta = 180
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.16 {
       astart = 270
       thickness = 20.0mil
       width = 176.0mil
       height = 176.0mil
       ha:attributes {
       }
       x = 4.72948mm
       y = 4.72948mm
       adelta = 180
       ha:flags {
       }
       clearance = 0.0
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 0.25908mm
       y = -1.26492mm
       rot = 0.000000
       string = %a.parent.refdes%
       fid = 0
       ha:flags {
        dyntext = 1
        floater = 1
       }
      }
     }
     ha:combining {
     }
    }
    ha:subc-aux {
     lid = 1
     ha:type {
      top = 1
      misc = 1
      virtual = 1
     }
     li:objects {
      ha:line.17 {
       clearance = 0.0
       y2 = 4.72948mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 9.87298mm
       x2 = 9.87298mm
       ha:flags {
       }
       y1 = 4.72948mm
      }
      ha:line.20 {
       clearance = 0.0
       y2 = 4.72948mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 3.71348mm
       x2 = 3.71348mm
       ha:flags {
       }
       y1 = 4.72948mm
      }
      ha:line.23 {
       clearance = 0.0
       y2 = 4.72948mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 3.71348mm
       x2 = 4.71348mm
       ha:flags {
       }
       y1 = 4.72948mm
      }
      ha:line.26 {
       clearance = 0.0
       y2 = 5.72948mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 3.71348mm
       x2 = 3.71348mm
       ha:flags {
       }
       y1 = 4.72948mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = +FiihmhDvVGqTubuzyoAAAAB
  ha:flags {
  }
 }
}
