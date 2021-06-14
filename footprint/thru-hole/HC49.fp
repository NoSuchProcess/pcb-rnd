li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = HC49 Crystals
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 28.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
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
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
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
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
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
        -33.0mil
        -33.0mil
        33.0mil
        -33.0mil
        33.0mil
        33.0mil
        -33.0mil
        33.0mil
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
        -33.0mil
        -33.0mil
        33.0mil
        -33.0mil
        33.0mil
        33.0mil
        -33.0mil
        33.0mil
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
     hdia = 28.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 60.0mil
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
        dia = 60.0mil
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
        dia = 60.0mil
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
        dia = 66.0mil
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
        dia = 66.0mil
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
     x = 60.2mil
     rot = 0.000000
     y = 60.2mil
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
     x = 6.60908mm
     rot = 0.000000
     y = 60.2mil
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
       x1 = 60.2mil
       x2 = 6.60908mm
       ha:flags {
       }
       y1 = 0.25908mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 110.2mil
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 6.60908mm
       x2 = 60.2mil
       ha:flags {
       }
       y1 = 110.2mil
      }
      ha:arc.12 {
       astart = 90
       thickness = 20.0mil
       width = 50.0mil
       height = 50.0mil
       ha:attributes {
       }
       x = 6.60908mm
       y = 60.2mil
       adelta = 180
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.16 {
       astart = 270
       thickness = 20.0mil
       width = 50.0mil
       height = 50.0mil
       ha:attributes {
       }
       x = 60.2mil
       y = 60.2mil
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
       y2 = 60.2mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 4.06908mm
       x2 = 4.06908mm
       ha:flags {
       }
       y1 = 60.2mil
      }
      ha:line.20 {
       clearance = 0.0
       y2 = 60.2mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 60.2mil
       x2 = 60.2mil
       ha:flags {
       }
       y1 = 60.2mil
      }
      ha:line.23 {
       clearance = 0.0
       y2 = 60.2mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 60.2mil
       x2 = 2.52908mm
       ha:flags {
       }
       y1 = 60.2mil
      }
      ha:line.26 {
       clearance = 0.0
       y2 = 2.52908mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 60.2mil
       x2 = 60.2mil
       ha:flags {
       }
       y1 = 60.2mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = K5zHll9kWHYvk+K8WxUAAAAB
  ha:flags {
  }
 }
}
