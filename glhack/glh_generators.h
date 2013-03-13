/** \file glh_generators.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

namespace glh{

double simplex_noise(double x, double y);
double simplex_noise(double x, double y, double z);
double simplex_noise(double x, double y, double z, double w);

} // namespace glh

