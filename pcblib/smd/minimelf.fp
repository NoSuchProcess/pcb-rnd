li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = minimelf
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
        -0.899922mm
        -0.649986mm
        -0.899922mm
        -0.649986mm
        0.899922mm
        0.649986mm
        0.899922mm
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
        38.09mil
        -1.217422mm
        -38.09mil
        -1.217422mm
        -38.09mil
        1.217422mm
        38.09mil
        1.217422mm
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
        -0.899922mm
        -0.649986mm
        -0.899922mm
        -0.649986mm
        0.899922mm
        0.649986mm
        0.899922mm
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
     }
     proto = 0
     xmirror = 0
     x = 1.52756in
     rot = 0.000000
     y = 35.999928mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 25.0mil
    }
    ha:padstack_ref.8 {
     smirror = 0
     ha:attributes {
      term = 2
     }
     proto = 0
     xmirror = 0
     x = 1.38582in
     rot = 0.000000
     y = 35.999928mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 25.0mil
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
       y2 = 1.38582in
       thickness = 7.87mil
       ha:attributes {
       }
       x1 = 35.999928mm
       x2 = 37.999924mm
       ha:flags {
       }
       y1 = 1.38582in
      }
      ha:line.12 {
       clearance = 0.0
       y2 = 1.44882in
       thickness = 7.87mil
       ha:attributes {
       }
       x1 = 35.999928mm
       x2 = 37.999924mm
       ha:flags {
       }
       y1 = 1.44882in
      }
      ha:text.6 {
       scale = 70
       ha:attributes {
       }
       x = 1.41949in
       y = 35.46983mm
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
       y2 = 35.999928mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 1.45669in
       x2 = 1.45669in
       ha:flags {
       }
       y1 = 35.999928mm
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 35.999928mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 1.45669in
       x2 = 1.45669in
       ha:flags {
       }
       y1 = 35.999928mm
      }
      ha:line.21 {
       clearance = 0.0
       y2 = 35.999928mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 1.45669in
       x2 = 35.999926mm
       ha:flags {
       }
       y1 = 35.999928mm
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 34.999928mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 1.45669in
       x2 = 1.45669in
       ha:flags {
       }
       y1 = 35.999928mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = kBcrKTtaM9kTfl/PmukAAAAB
  ha:flags {
  }
 }
}
