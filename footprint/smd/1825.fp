li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = 1825 Standard SMT resistor, capacitor etc
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
        31.495mil
        -3.399917mm
        -31.495mil
        -3.399917mm
        -31.495mil
        3.399917mm
        31.495mil
        3.399917mm
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
        0.876173mm
        -3.476117mm
        -0.876173mm
        -3.476117mm
        -0.876173mm
        3.476117mm
        0.876173mm
        3.476117mm
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
        31.495mil
        -3.399917mm
        -31.495mil
        -3.399917mm
        -31.495mil
        3.399917mm
        31.495mil
        3.399917mm
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
     x = 1.130173mm
     rot = 0.000000
     y = 3.730117mm
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
     x = 5.130165mm
     rot = 0.000000
     y = 3.730117mm
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
       y2 = 13.005mil
       thickness = 8.0mil
       ha:attributes {
       }
       x1 = 91.745mil
       x2 = 3.930015mm
       ha:flags {
       }
       y1 = 13.005mil
      }
      ha:line.12 {
       clearance = 0.0
       y2 = 7.129907mm
       thickness = 8.0mil
       ha:attributes {
       }
       x1 = 91.745mil
       x2 = 3.930015mm
       ha:flags {
       }
       y1 = 7.129907mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 2.330069mm
       y = 115.355mil
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
       y2 = 3.730117mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 3.130169mm
       x2 = 3.130169mm
       ha:flags {
       }
       y1 = 3.730117mm
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 3.730117mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 3.130169mm
       x2 = 3.130169mm
       ha:flags {
       }
       y1 = 3.730117mm
      }
      ha:line.21 {
       clearance = 0.0
       y2 = 3.730117mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 3.130169mm
       x2 = 4.130169mm
       ha:flags {
       }
       y1 = 3.730117mm
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 4.730117mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 3.130169mm
       x2 = 3.130169mm
       ha:flags {
       }
       y1 = 3.730117mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = k6ig2fGd0J0pkK+aZVsAAAAB
  ha:flags {
  }
 }
}
