li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = 1206 Standard SMT resistor, capacitor etc
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
        -0.94996mm
        -0.649986mm
        -0.94996mm
        -0.649986mm
        0.94996mm
        0.649986mm
        0.94996mm
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
        -1.02616mm
        -0.726186mm
        -1.02616mm
        -0.726186mm
        1.02616mm
        0.726186mm
        1.02616mm
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
        -0.94996mm
        -0.649986mm
        -0.94996mm
        -0.649986mm
        0.94996mm
        0.649986mm
        0.94996mm
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
     y = 1.28016mm
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
     x = 3.979926mm
     rot = 0.000000
     y = 1.28016mm
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
       y2 = 13.0mil
       thickness = 8.0mil
       ha:attributes {
       }
       x1 = 1.880108mm
       x2 = 121.26mil
       ha:flags {
       }
       y1 = 13.0mil
      }
      ha:line.12 {
       clearance = 0.0
       y2 = 2.23012mm
       thickness = 8.0mil
       ha:attributes {
       }
       x1 = 1.880108mm
       x2 = 121.26mil
       ha:flags {
       }
       y1 = 2.23012mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 66.14mil
       y = 0.48006mm
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
       y2 = 1.28016mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 97.64mil
       x2 = 97.64mil
       ha:flags {
       }
       y1 = 1.28016mm
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 1.28016mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 97.64mil
       x2 = 97.64mil
       ha:flags {
       }
       y1 = 1.28016mm
      }
      ha:line.21 {
       clearance = 0.0
       y2 = 1.28016mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 97.64mil
       x2 = 3.480056mm
       ha:flags {
       }
       y1 = 1.28016mm
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 2.28016mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 97.64mil
       x2 = 97.64mil
       ha:flags {
       }
       y1 = 1.28016mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = pjaLj8tSfYLVqy1mjLYAAAAB
  ha:flags {
  }
 }
}
