/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include <math.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/math_helper.h>
#include <librnd/core/actions.h>
#include "impedance.h"

#define TOM(x) RND_COORD_TO_MM(x)
#define SQR(x) ((x)*(x))
#define N0 377

/* Formulas from: https://www.eeweb.com/tools/microstrip-impedance
   ref: "The source of this formula is based on Wheeler's equation." */
double pcb_impedance_microstrip(rnd_coord_t trace_width, rnd_coord_t trace_height, rnd_coord_t subst_height, double dielect)
{
	double w = TOM(trace_width), t = TOM(trace_height), h = TOM(subst_height), Er = dielect;
	double weff, x1, x2;

	weff = w + (t/M_PI) * log(4*M_E / sqrt(SQR(t/h)+SQR(t/(w*M_PI+1.1*t*M_PI)))) * (Er+1)/(2*Er);
	x1 = 4 * ((14*Er+8)/(11*Er)) * (h/weff);
	x2 = sqrt(16 * SQR(h/weff) * SQR((14*Er+8)/(11*Er)) + ((Er+1)/(2*Er))*M_PI*M_PI);
	return N0 / (2*M_PI * sqrt(2) * sqrt(Er+1)) * log(1+4*(h/weff)*(x1+x2));
}


const char pcb_acts_impedance_microstrip[] = "impedance_microstrip(trace_width, trace_height, subst_height, dielectric)";
const char pcb_acth_impedance_microstrip[] = "Calculate the approximated impedance of a microstrip transmission line, in ohms";
fgw_error_t pcb_act_impedance_microstrip(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t trace_width, trace_height, subst_height;
	double dielectric;

	RND_ACT_CONVARG(1, FGW_COORD, impedance_microstrip, trace_width = fgw_coord(&argv[1]));
	RND_ACT_CONVARG(2, FGW_COORD, impedance_microstrip, trace_height = fgw_coord(&argv[2]));
	RND_ACT_CONVARG(3, FGW_COORD, impedance_microstrip, subst_height = fgw_coord(&argv[3]));
	RND_ACT_CONVARG(4, FGW_DOUBLE, impedance_microstrip, dielectric = argv[4].val.nat_double);

	res->type = FGW_DOUBLE;
	res->val.nat_double = pcb_impedance_microstrip(trace_width, trace_height, subst_height, dielectric);
	return 0;
}


/* As documented at https://chemandy.com/calculators/elliptical-integrals-of-the-first-kind-calculator.htm
   refrerence: These recursive equations were originally given for use in calculating the inductance of helical coils and are taken from Miller, H Craig, "Inductance Equation for a Single-Layer Circular Coil" Proceedings of the IEEE, Vol. 75, No 2, February 1987, pp. 256-257.
*/
static double ellint(double k)
{
	double a = 1.0, b = sqrt(1.0-k*k), c = k, K, lastc = 100;

	if ((k <= -1.0) || (k >= 1.0))
		return 0;

	for(K = M_PI/(2*a); (c != 0) && (c != lastc);) {
		double an, bn, cn;

		an = (a+b)/2;
		bn = sqrt(a*b);
		cn = (a-b)/2;
		lastc = c;
		a = an; b = bn; c = cn;
		K = M_PI/(2*a);
	}
	return K;
}

/* Formula from https://chemandy.com/calculators/coplanar-waveguide-with-ground-calculator.htm
   ref: Transmission Line Design Handbook by Brian C Wadell, Artech House 1991 page 79 */
double pcb_impedance_coplanar_waveguide(rnd_coord_t trace_width, rnd_coord_t trace_clearance, rnd_coord_t subst_height, double dielect)
{
	double a = TOM(trace_width), clr = TOM(trace_clearance), h = TOM(subst_height), Er = dielect;
	double b = clr+a+clr, eeff, ktmp, k, k_, kl, kl_, ktmp2;

	k = a/b;
	k_ = sqrt(1.0 - k*k);
	kl = tanh((M_PI * a)/(4.0*h)) / tanh((M_PI * b)/(4.0*h));
	kl_ = sqrt(1.0 - kl*kl);

	k = ellint(k);
	k_ = ellint(k_);
	kl = ellint(kl);
	kl_ = ellint(kl_);

	ktmp = (k_ * kl)  / (k * kl_);
	eeff = (1.0 + Er * ktmp) / (1.0 + ktmp);

	ktmp2 = k / k_ + kl / kl_;
	return ((60.0*M_PI) / sqrt(eeff)) * (1.0 / ktmp2);
}

const char pcb_acts_impedance_coplanar_waveguide[] = "impedance_coplanar_waveguide(trace_width, trace_clearance, subst_height, dielectric)";
const char pcb_acth_impedance_coplanar_waveguide[] = "Calculate the approximated impedance of a coplanar_waveguide transmission line, in ohms";
fgw_error_t pcb_act_impedance_coplanar_waveguide(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t trace_width, trace_clearance, subst_height;
	double dielectric;

	RND_ACT_CONVARG(1, FGW_COORD, impedance_coplanar_waveguide, trace_width = fgw_coord(&argv[1]));
	RND_ACT_CONVARG(2, FGW_COORD, impedance_coplanar_waveguide, trace_clearance = fgw_coord(&argv[2]));
	RND_ACT_CONVARG(3, FGW_COORD, impedance_coplanar_waveguide, subst_height = fgw_coord(&argv[3]));
	RND_ACT_CONVARG(4, FGW_DOUBLE, impedance_coplanar_waveguide, dielectric = argv[4].val.nat_double);

	res->type = FGW_DOUBLE;
	res->val.nat_double = pcb_impedance_coplanar_waveguide(trace_width, trace_clearance, subst_height, dielectric);
	return 0;
}
