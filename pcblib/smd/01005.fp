li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = 01005 Standard SMT resistor, capacitor etc
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
        4.92mil
        -5.11mil
        -4.92mil
        -5.11mil
        -4.92mil
        5.11mil
        4.92mil
        5.11mil
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
        7.92mil
        -0.205994mm
        -7.92mil
        -0.205994mm
        -7.92mil
        0.205994mm
        7.92mil
        0.205994mm
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
        4.92mil
        -5.11mil
        -4.92mil
        -5.11mil
        -4.92mil
        5.11mil
        4.92mil
        5.11mil
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
    ha:padstack_ref.7 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 17.92mil
     rot = 0.000000
     y = 0.459994mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
    }
    ha:padstack_ref.8 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 0
     xmirror = 0
     x = 34.06mil
     rot = 0.000000
     y = 0.459994mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
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
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = -0.139954mm
       y = -13.39mil
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
      ha:line.9 {
       clearance = 0.0
       y2 = 0.459994mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 0.660146mm
       x2 = 0.660146mm
       ha:flags {
       }
       y1 = 0.459994mm
      }
      ha:line.12 {
       clearance = 0.0
       y2 = 0.459994mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 0.660146mm
       x2 = 0.660146mm
       ha:flags {
       }
       y1 = 0.459994mm
      }
      ha:line.15 {
       clearance = 0.0
       y2 = 0.459994mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 0.660146mm
       x2 = 1.660146mm
       ha:flags {
       }
       y1 = 0.459994mm
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 1.459994mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 0.660146mm
       x2 = 0.660146mm
       ha:flags {
       }
       y1 = 0.459994mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = Q1c2peLxTgOy7O9YZ/8AAAAB
  ha:flags {
  }
 }
}
