#include <cmath>
#include "sunposition.h"

// Credits go to https://github.com/mourner/suncalc for the code below.

static const double daySec = 60 * 60 * 24;
static const double J1970 = 2440588;
static const double J2000 = 2451545;
static const double J0 = 0.0009;

static const double rad  = M_PI / 180.0f;
static const double e = rad * 23.4397f; // obliquity of the Earth

static double declination(double l, double b)    {
	return asin(sin(b) * cos(e) + cos(b) * sin(e) * sin(l));
}

static double solarMeanAnomaly(double d) {
	return rad * (357.5291f + 0.98560028f * d);
}

static double eclipticLongitude(double M) {
    double C = rad * (1.9148f * sin(M) + 0.02f * sin(2.0f * M) + 0.0003f * sin(3.0f * M)); // equation of center
    double P = rad * 102.9372f; // perihelion of the Earth

    return M + C + P + M_PI;
}

static double toJulian(time_t date) {
	return date / daySec - 0.5f + J1970;
}
static time_t fromJulian(double j)  {
	return (j + 0.5f - J1970) * daySec;
}
static double toDays(time_t date) {
	return toJulian(date) - J2000;
}

static double julianCycle(double d, double lw) {
	return round(d - J0 - lw / (2.0f * M_PI));
}

static double approxTransit(double Ht, double lw, double n) {
	return J0 + (Ht + lw) / (2.0f * M_PI) + n;
}
static double solarTransitJ(double ds, double M, double L)  {
	return J2000 + ds + 0.0053f * sin(M) - 0.0069f * sin(2 * L);
}

static double hourAngle(double h, double phi, double d) {
	return acos((sin(h) - sin(phi) * sin(d)) / (cos(phi) * cos(d)));
}

static double getSetJ(double h, double lw, double phi, double dec, double n, double M, double L) {

    double w = hourAngle(h, phi, dec),
        a = approxTransit(w, lw, n);

    return solarTransitJ(a, M, L);
}

bool SunPosition::getTimes(std::pair<time_t, time_t>& times, time_t date, double lat, double lng) {
	double lw = rad * -lng;
    double phi = rad * lat;

    double d = toDays(date);
    double n = julianCycle(d, lw);
    double ds = approxTransit(0, lw, n);

    double M = solarMeanAnomaly(ds);
    double L = eclipticLongitude(M);
    double dec = declination(L, 0);

    double Jnoon = solarTransitJ(ds, M, L);

    double Jset = getSetJ(-0.833f * rad, lw, phi, dec, n, M, L);
    double Jrise = Jnoon - (Jset - Jnoon);

    times = std::make_pair<time_t, time_t>(fromJulian(Jrise), fromJulian(Jset));

    return true;
}
