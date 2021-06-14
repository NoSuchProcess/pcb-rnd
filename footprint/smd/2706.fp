li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = 2706 Standard SMT resistor, capacitor etc
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
        39.0mil
        -42.0mil
        -39.0mil
        -42.0mil
        -39.0mil
        42.0mil
        39.0mil
        42.0mil
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
        42.0mil
        -45.0mil
        -42.0mil
        -45.0mil
        -42.0mil
        45.0mil
        42.0mil
        45.0mil
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
        39.0mil
        -42.0mil
        -39.0mil
        -42.0mil
        -39.0mil
        42.0mil
        39.0mil
        42.0mil
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
     x = 1.422792mm
     rot = 0.000000
     y = 1.498992mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
    }
    ha:padstack_ref.20 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 0
     xmirror = 0
     x = 6.909192mm
     rot = 0.000000
     y = 1.498992mm
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
      ha:line.7 {
       clearance = 0.0
       y2 = 2.870592mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.127392mm
       x2 = 0.127392mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 2.870592mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.127392mm
       x2 = 8.204592mm
       ha:flags {
       }
       y1 = 2.870592mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 8.204592mm
       x2 = 8.204592mm
       ha:flags {
       }
       y1 = 2.870592mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 8.204592mm
       x2 = 0.127392mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 8.712592mm
       y = 1.498992mm
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
      ha:line.21 {
       clearance = 0.0
       y2 = 1.498992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 4.165992mm
       x2 = 4.165992mm
       ha:flags {
       }
       y1 = 1.498992mm
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 1.498992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 4.165992mm
       x2 = 4.165992mm
       ha:flags {
       }
       y1 = 1.498992mm
      }
      ha:line.27 {
       clearance = 0.0
       y2 = 1.498992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 4.165992mm
       x2 = 5.165992mm
       ha:flags {
       }
       y1 = 1.498992mm
      }
      ha:line.30 {
       clearance = 0.0
       y2 = 2.498992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 4.165992mm
       x2 = 4.165992mm
       ha:flags {
       }
       y1 = 1.498992mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = FePvr11aAXKyPT8fC0AAAAAB
  ha:flags {
  }
 }
}
