li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = TANT_B Tantalum SMT capacitor (pin 1 is +)
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
        35.5mil
        -76.5mil
        -35.5mil
        -76.5mil
        -35.5mil
        76.5mil
        35.5mil
        76.5mil
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
        38.5mil
        -79.5mil
        -38.5mil
        -79.5mil
        -38.5mil
        79.5mil
        38.5mil
        79.5mil
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
        35.5mil
        -76.5mil
        -35.5mil
        -76.5mil
        -35.5mil
        76.5mil
        35.5mil
        76.5mil
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
    ha:padstack_ref.25 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 1.956583mm
     rot = 0.000000
     y = 2.641992mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
    }
    ha:padstack_ref.26 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 0
     xmirror = 0
     x = 4.750583mm
     rot = 0.000000
     y = 2.641992mm
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
       y2 = 4.597792mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.686192mm
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 5.156592mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 1.143783mm
       ha:flags {
       }
       y1 = 4.597792mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 5.156592mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 1.143783mm
       x2 = 6.198383mm
       ha:flags {
       }
       y1 = 5.156592mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 6.198383mm
       x2 = 6.198383mm
       ha:flags {
       }
       y1 = 5.156592mm
      }
      ha:line.19 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 6.198383mm
       x2 = 1.143783mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:line.22 {
       clearance = 0.0
       y2 = 0.686192mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 1.143783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 6.706383mm
       y = 2.641992mm
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
      ha:line.27 {
       clearance = 0.0
       y2 = 2.641992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 3.353583mm
       x2 = 3.353583mm
       ha:flags {
       }
       y1 = 2.641992mm
      }
      ha:line.30 {
       clearance = 0.0
       y2 = 2.641992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 3.353583mm
       x2 = 3.353583mm
       ha:flags {
       }
       y1 = 2.641992mm
      }
      ha:line.33 {
       clearance = 0.0
       y2 = 2.641992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 3.353583mm
       x2 = 4.353583mm
       ha:flags {
       }
       y1 = 2.641992mm
      }
      ha:line.36 {
       clearance = 0.0
       y2 = 3.641992mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 3.353583mm
       x2 = 3.353583mm
       ha:flags {
       }
       y1 = 2.641992mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = z+x6iElAGsr55qcSOkgAAAAB
  ha:flags {
  }
 }
}
