li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = 0805 Standard SMT resistor, capacitor etc
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
        0.649986mm
        -0.749808mm
        -0.649986mm
        -0.749808mm
        -0.649986mm
        0.749808mm
        0.649986mm
        0.749808mm
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
        0.726186mm
        -32.52mil
        -0.726186mm
        -32.52mil
        -0.726186mm
        32.52mil
        0.726186mm
        32.52mil
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
        0.649986mm
        -0.749808mm
        -0.649986mm
        -0.749808mm
        -0.649986mm
        0.749808mm
        0.649986mm
        0.749808mm
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
     x = 38.59mil
     rot = 0.000000
     y = 42.52mil
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
     x = 109.45mil
     rot = 0.000000
     y = 42.52mil
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
      ha:line.9 {
       clearance = 0.0
       y2 = 14.97mil
       thickness = 8.0mil
       ha:attributes {
       }
       x1 = 70.09mil
       x2 = 77.95mil
       ha:flags {
       }
       y1 = 14.97mil
      }
      ha:line.12 {
       clearance = 0.0
       y2 = 1.779778mm
       thickness = 8.0mil
       ha:attributes {
       }
       x1 = 70.09mil
       x2 = 77.95mil
       ha:flags {
       }
       y1 = 1.779778mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 42.52mil
       y = 0.279908mm
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
      ha:line.15 {
       clearance = 0.0
       y2 = 42.52mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 1.880108mm
       x2 = 1.880108mm
       ha:flags {
       }
       y1 = 42.52mil
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 42.52mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 1.880108mm
       x2 = 1.880108mm
       ha:flags {
       }
       y1 = 42.52mil
      }
      ha:line.21 {
       clearance = 0.0
       y2 = 42.52mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 1.880108mm
       x2 = 2.880108mm
       ha:flags {
       }
       y1 = 42.52mil
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 2.080008mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 1.880108mm
       x2 = 1.880108mm
       ha:flags {
       }
       y1 = 42.52mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = Gb4CEJNqF8FflGx6irgAAAAB
  ha:flags {
  }
 }
}
