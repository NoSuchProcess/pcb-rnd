li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = DSUB connector, female/male
   openscad = DSUB.scad
   openscad-param = {pins=37,gender=0,rotation=270}
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 35.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
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
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
       }
       ha:layer_mask {
        bottom = 1
        copper = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
       }
       ha:layer_mask {
        copper = 1
        intern = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -33.0mil
        -33.0mil
        33.0mil
        -33.0mil
        33.0mil
        33.0mil
        -33.0mil
        33.0mil
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
        -33.0mil
        -33.0mil
        33.0mil
        -33.0mil
        33.0mil
        33.0mil
        -33.0mil
        33.0mil
       }
       ha:layer_mask {
        bottom = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 1
    }
    ha:ps_proto_v6.1 {
     htop = 0
     hdia = 35.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 60.0mil
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
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 60.0mil
       }
       ha:layer_mask {
        bottom = 1
        copper = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 60.0mil
       }
       ha:layer_mask {
        copper = 1
        intern = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 66.0mil
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
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 66.0mil
       }
       ha:layer_mask {
        bottom = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 1
    }
    ha:ps_proto_v6.2 {
     htop = 0
     hdia = 125.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 250.0mil
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
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 250.0mil
       }
       ha:layer_mask {
        bottom = 1
        copper = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 250.0mil
       }
       ha:layer_mask {
        copper = 1
        intern = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 256.0mil
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
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 256.0mil
       }
       ha:layer_mask {
        bottom = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 1
    }
   }
   li:objects {
    ha:padstack_ref.43 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 1.27in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.47 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 1.378in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.51 {
     smirror = 0
     ha:attributes {
      term = 3
      name = 3
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 1.486in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.55 {
     smirror = 0
     ha:attributes {
      term = 4
      name = 4
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 1.594in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.59 {
     smirror = 0
     ha:attributes {
      term = 5
      name = 5
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 1.702in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.63 {
     smirror = 0
     ha:attributes {
      term = 6
      name = 6
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 1.81in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.67 {
     smirror = 0
     ha:attributes {
      term = 7
      name = 7
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 1.918in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.71 {
     smirror = 0
     ha:attributes {
      term = 8
      name = 8
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.026in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.75 {
     smirror = 0
     ha:attributes {
      term = 9
      name = 9
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 54.2036mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.79 {
     smirror = 0
     ha:attributes {
      term = 10
      name = 10
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.242in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.83 {
     smirror = 0
     ha:attributes {
      term = 11
      name = 11
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.35in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.87 {
     smirror = 0
     ha:attributes {
      term = 12
      name = 12
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.458in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.91 {
     smirror = 0
     ha:attributes {
      term = 13
      name = 13
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 65.1764mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.95 {
     smirror = 0
     ha:attributes {
      term = 14
      name = 14
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.674in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.99 {
     smirror = 0
     ha:attributes {
      term = 15
      name = 15
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.782in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.103 {
     smirror = 0
     ha:attributes {
      term = 16
      name = 16
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.89in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.107 {
     smirror = 0
     ha:attributes {
      term = 17
      name = 17
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 2.998in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.111 {
     smirror = 0
     ha:attributes {
      term = 18
      name = 18
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 78.8924mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.115 {
     smirror = 0
     ha:attributes {
      term = 19
      name = 19
     }
     proto = 1
     xmirror = 0
     x = 1.056in
     rot = 0.000000
     y = 81.6356mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.119 {
     smirror = 0
     ha:attributes {
      term = 20
      name = 20
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 1.324in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.123 {
     smirror = 0
     ha:attributes {
      term = 21
      name = 21
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 36.3728mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.127 {
     smirror = 0
     ha:attributes {
      term = 22
      name = 22
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 1.54in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.131 {
     smirror = 0
     ha:attributes {
      term = 23
      name = 23
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 1.648in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.135 {
     smirror = 0
     ha:attributes {
      term = 24
      name = 24
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 1.756in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.139 {
     smirror = 0
     ha:attributes {
      term = 25
      name = 25
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 1.864in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.143 {
     smirror = 0
     ha:attributes {
      term = 26
      name = 26
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 1.972in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.147 {
     smirror = 0
     ha:attributes {
      term = 27
      name = 27
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 2.08in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.151 {
     smirror = 0
     ha:attributes {
      term = 28
      name = 28
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 2.188in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.155 {
     smirror = 0
     ha:attributes {
      term = 29
      name = 29
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 2.296in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.159 {
     smirror = 0
     ha:attributes {
      term = 30
      name = 30
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 61.0616mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.163 {
     smirror = 0
     ha:attributes {
      term = 31
      name = 31
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 2.512in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.167 {
     smirror = 0
     ha:attributes {
      term = 32
      name = 32
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 2.62in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.171 {
     smirror = 0
     ha:attributes {
      term = 33
      name = 33
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 2.728in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.175 {
     smirror = 0
     ha:attributes {
      term = 34
      name = 34
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 72.0344mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.179 {
     smirror = 0
     ha:attributes {
      term = 35
      name = 35
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 2.944in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.183 {
     smirror = 0
     ha:attributes {
      term = 36
      name = 36
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 3.052in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.187 {
     smirror = 0
     ha:attributes {
      term = 37
      name = 37
     }
     proto = 1
     xmirror = 0
     x = 944.0mil
     rot = 0.000000
     y = 3.16in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.191 {
     smirror = 0
     ha:attributes {
      term = 38
      name = C1
     }
     proto = 2
     xmirror = 0
     x = 1000.0mil
     rot = 0.000000
     y = 1000.0mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.192 {
     smirror = 0
     ha:attributes {
      term = 39
      name = C2
     }
     proto = 2
     xmirror = 0
     x = 1000.0mil
     rot = 0.000000
     y = 3.484in
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
       y2 = 880.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 635.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 880.0mil
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 3.604in
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 665.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 880.0mil
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 3.604in
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 665.0mil
       x2 = 635.0mil
       ha:flags {
       }
       y1 = 3.604in
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 880.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 635.0mil
       x2 = 635.0mil
       ha:flags {
       }
       y1 = 3.604in
      }
      ha:line.19 {
       clearance = 0.0
       y2 = 940.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 635.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 940.0mil
      }
      ha:line.22 {
       clearance = 0.0
       y2 = 1.06in
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 635.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 1.06in
      }
      ha:line.25 {
       clearance = 0.0
       y2 = 3.544in
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 635.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 3.544in
      }
      ha:line.28 {
       clearance = 0.0
       y2 = 86.9696mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 635.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 86.9696mm
      }
      ha:line.31 {
       clearance = 0.0
       y2 = 1.11in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 665.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.11in
      }
      ha:line.34 {
       clearance = 0.0
       y2 = 3.374in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 770.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.11in
      }
      ha:line.37 {
       clearance = 0.0
       y2 = 3.374in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 770.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 3.374in
      }
      ha:line.40 {
       clearance = 0.0
       y2 = 1.11in
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 665.0mil
       x2 = 665.0mil
       ha:flags {
       }
       y1 = 3.374in
      }
      ha:line.44 {
       clearance = 0.0
       y2 = 1.27in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.27in
      }
      ha:line.48 {
       clearance = 0.0
       y2 = 1.378in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.378in
      }
      ha:line.52 {
       clearance = 0.0
       y2 = 1.486in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.486in
      }
      ha:line.56 {
       clearance = 0.0
       y2 = 1.594in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.594in
      }
      ha:line.60 {
       clearance = 0.0
       y2 = 1.702in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.702in
      }
      ha:line.64 {
       clearance = 0.0
       y2 = 1.81in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.81in
      }
      ha:line.68 {
       clearance = 0.0
       y2 = 1.918in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.918in
      }
      ha:line.72 {
       clearance = 0.0
       y2 = 2.026in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.026in
      }
      ha:line.76 {
       clearance = 0.0
       y2 = 54.2036mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 54.2036mm
      }
      ha:line.80 {
       clearance = 0.0
       y2 = 2.242in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.242in
      }
      ha:line.84 {
       clearance = 0.0
       y2 = 2.35in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.35in
      }
      ha:line.88 {
       clearance = 0.0
       y2 = 2.458in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.458in
      }
      ha:line.92 {
       clearance = 0.0
       y2 = 65.1764mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 65.1764mm
      }
      ha:line.96 {
       clearance = 0.0
       y2 = 2.674in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.674in
      }
      ha:line.100 {
       clearance = 0.0
       y2 = 2.782in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.782in
      }
      ha:line.104 {
       clearance = 0.0
       y2 = 2.89in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.89in
      }
      ha:line.108 {
       clearance = 0.0
       y2 = 2.998in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.998in
      }
      ha:line.112 {
       clearance = 0.0
       y2 = 78.8924mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 78.8924mm
      }
      ha:line.116 {
       clearance = 0.0
       y2 = 81.6356mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 1.016in
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 81.6356mm
      }
      ha:line.120 {
       clearance = 0.0
       y2 = 1.324in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.324in
      }
      ha:line.124 {
       clearance = 0.0
       y2 = 36.3728mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 36.3728mm
      }
      ha:line.128 {
       clearance = 0.0
       y2 = 1.54in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.54in
      }
      ha:line.132 {
       clearance = 0.0
       y2 = 1.648in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.648in
      }
      ha:line.136 {
       clearance = 0.0
       y2 = 1.756in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.756in
      }
      ha:line.140 {
       clearance = 0.0
       y2 = 1.864in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.864in
      }
      ha:line.144 {
       clearance = 0.0
       y2 = 1.972in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 1.972in
      }
      ha:line.148 {
       clearance = 0.0
       y2 = 2.08in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.08in
      }
      ha:line.152 {
       clearance = 0.0
       y2 = 2.188in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.188in
      }
      ha:line.156 {
       clearance = 0.0
       y2 = 2.296in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.296in
      }
      ha:line.160 {
       clearance = 0.0
       y2 = 61.0616mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 61.0616mm
      }
      ha:line.164 {
       clearance = 0.0
       y2 = 2.512in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.512in
      }
      ha:line.168 {
       clearance = 0.0
       y2 = 2.62in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.62in
      }
      ha:line.172 {
       clearance = 0.0
       y2 = 2.728in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.728in
      }
      ha:line.176 {
       clearance = 0.0
       y2 = 72.0344mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 72.0344mm
      }
      ha:line.180 {
       clearance = 0.0
       y2 = 2.944in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 2.944in
      }
      ha:line.184 {
       clearance = 0.0
       y2 = 3.052in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 3.052in
      }
      ha:line.188 {
       clearance = 0.0
       y2 = 3.16in
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 904.0mil
       x2 = 770.0mil
       ha:flags {
       }
       y1 = 3.16in
      }
      ha:text.6 {
       scale = 150
       ha:attributes {
       }
       x = 1000.0mil
       y = 81.6356mm
       rot = 90.000000
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
      ha:line.193 {
       clearance = 0.0
       y2 = 2.242in
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 25.436471mm
       x2 = 25.436471mm
       ha:flags {
       }
       y1 = 2.242in
      }
      ha:line.196 {
       clearance = 0.0
       y2 = 1.27in
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 26.67mm
       x2 = 26.67mm
       ha:flags {
       }
       y1 = 1.27in
      }
      ha:line.199 {
       clearance = 0.0
       y2 = 1.27in
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 26.67mm
       x2 = 27.67mm
       ha:flags {
       }
       y1 = 1.27in
      }
      ha:line.202 {
       clearance = 0.0
       y2 = 33.258mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 26.67mm
       x2 = 26.67mm
       ha:flags {
       }
       y1 = 1.27in
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = 4q1Up8irTZeVRbtKn8sAAAAB
  ha:flags {
  }
 }
}
