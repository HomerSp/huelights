#include <cmath>
#include "sunposition.h"

// Credits go to https://github.com/mourner/suncalc for the code below.

static const double daySec = 60 * 60 * 24;
static const double J1970 = 2440588;
static const double J2000 = 2451545;
static const double J0 = 0.0009;

static const double rad  = M_PI / 180.0f;
static const double e = rad * 23.4397f; // obliquity of the Earth


double rightAscension(double l, double b) {
	return atan2(sin(l) * cos(e) - tan(b) * sin(e), cos(l));
}
double declination(double l, double b)    {
	return asin(sin(b) * cos(e) + cos(b) * sin(e) * sin(l));
}

double azimuth(double H, double phi, double dec)  {
	return atan2(sin(H), cos(H) * sin(phi) - tan(dec) * cos(phi));
}
double altitude(double H, double phi, double dec) {
	return asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(H));
}

double siderealTime(double d , double lw) {
	return rad * (280.16f + 360.9856235f * d) - lw;
}

double astroRefraction(double h) {
    if (h < 0) // the following formula works for positive altitudes only.
        h = 0; // if h = -0.08901179 a div/0 would occur.

    // formula 16.4 of "Astronomical Algorithms" 2nd edition by Jean Meeus (Willmann-Bell, Richmond) 1998.
    // 1.02 / tan(h + 10.26 / (h + 5.10)) h in degrees, result in arc minutes -> converted to rad:
    return 0.0002967f / tan(h + 0.00312536f / (h + 0.08901179f));
}

double solarMeanAnomaly(double d) {
	return rad * (357.5291 + 0.98560028 * d);
}

double eclipticLongitude(double M) {
    double C = rad * (1.9148f * sin(M) + 0.02f * sin(2.0f * M) + 0.0003f * sin(3.0f * M)); // equation of center
    double P = rad * 102.9372; // perihelion of the Earth

    return M + C + P + M_PI;
}

double toJulian(time_t date) {
	return date / daySec - 0.5f + J1970;
}
time_t fromJulian(double j)  {
	return (j + 0.5f - J1970) * daySec;
}
double toDays(time_t date) {
	return toJulian(date) - J2000;
}

double julianCycle(double d, double lw) {
	return round(d - J0 - lw / (2.0f * M_PI));
}

double approxTransit(double Ht, double lw, double n) {
	return J0 + (Ht + lw) / (2.0f * M_PI) + n;
}
double solarTransitJ(double ds, double M, double L)  {
	return J2000 + ds + 0.0053f * sin(M) - 0.0069f * sin(2 * L);
}

double hourAngle(double h, double phi, double d) {
	return acos((sin(h) - sin(phi) * sin(d)) / (cos(phi) * cos(d)));
}

double getSetJ(double h, double lw, double phi, double dec, double n, double M, double L) {

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
