li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = SOT326 SMT transistor, 6 pins
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
        7.5mil
        -17.5mil
        -7.5mil
        -17.5mil
        -7.5mil
        17.5mil
        7.5mil
        17.5mil
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
        10.5mil
        -20.5mil
        -10.5mil
        -20.5mil
        -10.5mil
        20.5mil
        10.5mil
        20.5mil
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
        7.5mil
        -17.5mil
        -7.5mil
        -17.5mil
        -7.5mil
        17.5mil
        7.5mil
        17.5mil
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
     x = 25.5mil
     rot = 0.000000
     y = 105.5mil
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
     x = 51.5mil
     rot = 0.000000
     y = 105.5mil
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
     x = 76.5mil
     rot = 0.000000
     y = 105.5mil
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
     proto = 0
     xmirror = 0
     x = 76.5mil
     rot = 0.000000
     y = 35.5mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.23 {
     smirror = 0
     ha:attributes {
      term = 5
      name = 5
     }
     proto = 0
     xmirror = 0
     x = 51.5mil
     rot = 0.000000
     y = 35.5mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.24 {
     smirror = 0
     ha:attributes {
      term = 6
      name = 6
     }
     proto = 0
     xmirror = 0
     x = 25.5mil
     rot = 0.000000
     y = 35.5mil
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
       y2 = 130.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 11.5mil
       x2 = 11.5mil
       ha:flags {
       }
       y1 = 11.5mil
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 130.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 11.5mil
       x2 = 91.5mil
       ha:flags {
       }
       y1 = 130.5mil
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 11.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 91.5mil
       x2 = 91.5mil
       ha:flags {
       }
       y1 = 130.5mil
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 11.5mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 91.5mil
       x2 = 11.5mil
       ha:flags {
       }
       y1 = 11.5mil
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 111.5mil
       y = 11.5mil
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
      ha:line.25 {
       clearance = 0.0
       y2 = 70.5mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 1.299633mm
       x2 = 1.299633mm
       ha:flags {
       }
       y1 = 70.5mil
      }
      ha:line.28 {
       clearance = 0.0
       y2 = 105.5mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 25.5mil
       x2 = 25.5mil
       ha:flags {
       }
       y1 = 105.5mil
      }
      ha:line.31 {
       clearance = 0.0
       y2 = 3.6797mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 25.5mil
       x2 = 25.5mil
       ha:flags {
       }
       y1 = 105.5mil
      }
      ha:line.34 {
       clearance = 0.0
       y2 = 105.5mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 25.5mil
       x2 = 1.6477mm
       ha:flags {
       }
       y1 = 105.5mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = 6Bv/QN6fnFrzfC7tuHoAAAAB
  ha:flags {
  }
 }
}
