li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = SOT23D SMT diode (pin 1 is cathode)
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
        17.0mil
        -20.0mil
        -17.0mil
        -20.0mil
        -17.0mil
        20.0mil
        17.0mil
        20.0mil
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
        20.0mil
        -23.0mil
        -20.0mil
        -23.0mil
        -20.0mil
        23.0mil
        20.0mil
        23.0mil
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
        17.0mil
        -20.0mil
        -17.0mil
        -20.0mil
        -17.0mil
        20.0mil
        17.0mil
        20.0mil
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
      term = 2
      name = 2
     }
     proto = 0
     xmirror = 0
     x = 35.0mil
     rot = 0.000000
     y = 120.0mil
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
      term = 3
      name = 3
     }
     proto = 0
     xmirror = 0
     x = 113.0mil
     rot = 0.000000
     y = 120.0mil
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
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 74.0mil
     rot = 0.000000
     y = 38.0mil
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
       y2 = 149.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 10.0mil
       x2 = 10.0mil
       ha:flags {
       }
       y1 = 10.0mil
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 149.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 10.0mil
       x2 = 138.0mil
       ha:flags {
       }
       y1 = 149.0mil
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 10.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 138.0mil
       x2 = 138.0mil
       ha:flags {
       }
       y1 = 149.0mil
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 10.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 138.0mil
       x2 = 10.0mil
       ha:flags {
       }
       y1 = 10.0mil
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 158.0mil
       y = 10.0mil
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
      ha:line.22 {
       clearance = 0.0
       y2 = 2.353733mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 74.0mil
       x2 = 74.0mil
       ha:flags {
       }
       y1 = 2.353733mm
      }
      ha:line.25 {
       clearance = 0.0
       y2 = 120.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 35.0mil
       x2 = 35.0mil
       ha:flags {
       }
       y1 = 120.0mil
      }
      ha:line.28 {
       clearance = 0.0
       y2 = 120.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 35.0mil
       x2 = 1.889mm
       ha:flags {
       }
       y1 = 120.0mil
      }
      ha:line.31 {
       clearance = 0.0
       y2 = 4.048mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 35.0mil
       x2 = 35.0mil
       ha:flags {
       }
       y1 = 120.0mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = hksuSYo1TTefRrW2D28AAAAB
  ha:flags {
  }
 }
}
