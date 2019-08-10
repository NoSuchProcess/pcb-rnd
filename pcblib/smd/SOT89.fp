li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = SOT89 SMT transistor, 4 pins
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 0.0
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        18.5mil
        -30.5mil
        -18.5mil
        -30.5mil
        -18.5mil
        30.5mil
        18.5mil
        30.5mil
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
        21.5mil
        -33.5mil
        -21.5mil
        -33.5mil
        -21.5mil
        33.5mil
        21.5mil
        33.5mil
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
        18.5mil
        -30.5mil
        -18.5mil
        -30.5mil
        -18.5mil
        30.5mil
        18.5mil
        30.5mil
       }
       ha:layer_mask {
        top = 1
        paste = 1
       }
       ha:combining {
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 0
    }
    ha:ps_proto_v6.1 {
     htop = 0
     hdia = 0.0
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        60.5mil
        30.5mil
        60.5mil
        -30.5mil
        -60.5mil
        -30.5mil
        -60.5mil
        30.5mil
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
        63.5mil
        33.5mil
        63.5mil
        -33.5mil
        -63.5mil
        -33.5mil
        -63.5mil
        33.5mil
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
        60.5mil
        30.5mil
        60.5mil
        -30.5mil
        -60.5mil
        -30.5mil
        -60.5mil
        30.5mil
       }
       ha:layer_mask {
        top = 1
        paste = 1
       }
       ha:combining {
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 0
    }
   }
   li:objects {
    ha:padstack_ref.19 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 36.5mil
     rot = 0.000000
     y = 170.5mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.20 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 0
     xmirror = 0
     x = 97.5mil
     rot = 0.000000
     y = 170.5mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.21 {
     smirror = 0
     ha:attributes {
      term = 3
      name = 3
     }
     proto = 0
     xmirror = 0
     x = 158.5mil
     rot = 0.000000
     y = 170.5mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.22 {
     smirror = 0
     ha:attributes {
      term = 4
      name = 4
     }
     proto = 1
     xmirror = 0
     x = 97.5mil
     rot = 0.000000
     y = 48.5mil
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
      ha:line.7 {
       clearance = 0.0
       y2 = 213.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 6.5mil
       x2 = 6.5mil
       ha:flags {
       }
       y1 = 6.5mil
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 213.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 6.5mil
       x2 = 189.5mil
       ha:flags {
       }
       y1 = 213.5mil
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 6.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 189.5mil
       x2 = 189.5mil
       ha:flags {
       }
       y1 = 213.5mil
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 6.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 189.5mil
       x2 = 6.5mil
       ha:flags {
       }
       y1 = 6.5mil
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 209.5mil
       y = 6.5mil
       rot = 270.000000
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
      ha:line.23 {
       clearance = 0.0
       y2 = 140.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 97.5mil
       x2 = 97.5mil
       ha:flags {
       }
       y1 = 140.0mil
      }
      ha:line.26 {
       clearance = 0.0
       y2 = 170.5mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 36.5mil
       x2 = 36.5mil
       ha:flags {
       }
       y1 = 170.5mil
      }
      ha:line.29 {
       clearance = 0.0
       y2 = 5.3307mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 36.5mil
       x2 = 36.5mil
       ha:flags {
       }
       y1 = 170.5mil
      }
      ha:line.32 {
       clearance = 0.0
       y2 = 170.5mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 36.5mil
       x2 = 1.9271mm
       ha:flags {
       }
       y1 = 170.5mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = nhFNna5oWAROPukkSGYAAAAB
  ha:flags {
  }
 }
}
