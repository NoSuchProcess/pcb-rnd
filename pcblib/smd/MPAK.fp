li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = MPAK
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
        15.5mil
        -43.5mil
        -15.5mil
        -43.5mil
        -15.5mil
        43.5mil
        15.5mil
        43.5mil
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
        18.5mil
        -46.5mil
        -18.5mil
        -46.5mil
        -18.5mil
        46.5mil
        18.5mil
        46.5mil
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
        15.5mil
        -43.5mil
        -15.5mil
        -43.5mil
        -15.5mil
        43.5mil
        15.5mil
        43.5mil
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
        80.5mil
        43.5mil
        80.5mil
        -43.5mil
        -80.5mil
        -43.5mil
        -80.5mil
        43.5mil
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
        83.5mil
        46.5mil
        83.5mil
        -46.5mil
        -83.5mil
        -46.5mil
        -83.5mil
        46.5mil
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
        80.5mil
        43.5mil
        80.5mil
        -43.5mil
        -80.5mil
        -43.5mil
        -80.5mil
        43.5mil
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
     x = 0.940192mm
     rot = 0.000000
     y = 12.751192mm
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
     x = 2.210192mm
     rot = 0.000000
     y = 12.751192mm
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
     x = 3.480192mm
     rot = 0.000000
     y = 12.751192mm
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
     x = 4.750192mm
     rot = 0.000000
     y = 12.751192mm
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
     proto = 1
     xmirror = 0
     x = 2.845192mm
     rot = 0.000000
     y = 1.651392mm
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
       y2 = 14.300592mm
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
       y2 = 14.300592mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.127392mm
       x2 = 5.588392mm
       ha:flags {
       }
       y1 = 14.300592mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 5.588392mm
       x2 = 5.588392mm
       ha:flags {
       }
       y1 = 14.300592mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 5.588392mm
       x2 = 0.127392mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 6.096392mm
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
      ha:line.24 {
       clearance = 0.0
       y2 = 10.531232mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 2.845192mm
       x2 = 2.845192mm
       ha:flags {
       }
       y1 = 10.531232mm
      }
      ha:line.27 {
       clearance = 0.0
       y2 = 12.751192mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 0.940192mm
       x2 = 0.940192mm
       ha:flags {
       }
       y1 = 12.751192mm
      }
      ha:line.30 {
       clearance = 0.0
       y2 = 13.751192mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 0.940192mm
       x2 = 0.940192mm
       ha:flags {
       }
       y1 = 12.751192mm
      }
      ha:line.33 {
       clearance = 0.0
       y2 = 12.751192mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 0.940192mm
       x2 = 1.940192mm
       ha:flags {
       }
       y1 = 12.751192mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = eZwOg7hh3XSQ6QI9EysAAAAB
  ha:flags {
  }
 }
}
