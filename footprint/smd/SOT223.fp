li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = SOT223 SMT transistor, 4 pins
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
        28.0mil
        -61.0mil
        -28.0mil
        -61.0mil
        -28.0mil
        61.0mil
        28.0mil
        61.0mil
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
        31.0mil
        -64.0mil
        -31.0mil
        -64.0mil
        -31.0mil
        64.0mil
        31.0mil
        64.0mil
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
        28.0mil
        -61.0mil
        -28.0mil
        -61.0mil
        -28.0mil
        61.0mil
        28.0mil
        61.0mil
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
        106.0mil
        61.0mil
        106.0mil
        -61.0mil
        -106.0mil
        -61.0mil
        -106.0mil
        61.0mil
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
        109.0mil
        64.0mil
        109.0mil
        -64.0mil
        -109.0mil
        -64.0mil
        -109.0mil
        64.0mil
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
        106.0mil
        61.0mil
        106.0mil
        -61.0mil
        -106.0mil
        -61.0mil
        -106.0mil
        61.0mil
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
     x = 1.448192mm
     rot = 0.000000
     y = 8.483992mm
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
     x = 3.734192mm
     rot = 0.000000
     y = 8.483992mm
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
     x = 6.045592mm
     rot = 0.000000
     y = 8.483992mm
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
     x = 3.734192mm
     rot = 0.000000
     y = 2.286392mm
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
       y2 = 10.642992mm
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
       y2 = 10.642992mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.127392mm
       x2 = 7.366392mm
       ha:flags {
       }
       y1 = 10.642992mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 7.366392mm
       x2 = 7.366392mm
       ha:flags {
       }
       y1 = 10.642992mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 7.366392mm
       x2 = 0.127392mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 7.874392mm
       y = 0.127392mm
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
       y2 = 6.934592mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 3.740542mm
       x2 = 3.740542mm
       ha:flags {
       }
       y1 = 6.934592mm
      }
      ha:line.26 {
       clearance = 0.0
       y2 = 8.483992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 1.448192mm
       x2 = 1.448192mm
       ha:flags {
       }
       y1 = 8.483992mm
      }
      ha:line.29 {
       clearance = 0.0
       y2 = 9.483992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 1.448192mm
       x2 = 1.448192mm
       ha:flags {
       }
       y1 = 8.483992mm
      }
      ha:line.32 {
       clearance = 0.0
       y2 = 8.483992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 1.448192mm
       x2 = 2.448192mm
       ha:flags {
       }
       y1 = 8.483992mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = wL7+F9vz1VIcfqPPcnsAAAAB
  ha:flags {
  }
 }
}
