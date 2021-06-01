#include "sunrise_sunset.h"

#include <stdlib.h>
#include <math.h>

#define ZENITH 90.83333

#define RAD(x) ((x) * (M_PI / 180))
#define DEG(x) ((x) * (180 / M_PI))

// Algorithm used: https://web.archive.org/web/20161202180207/http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
// Also implemented in: https://github.com/jebeaudet/SunriseSunsetCalculator

// This mimicks the behaviour of Python's modulo operator when the numerator is negative
double neg_fmod(double a, double b) {
	double val = fmod(a, b);
	if (val < 0) {
		return b + val;
	}
	return val;
}

time_t calculate_sunrise(const struct tm *now, int local_offset) {
	const double lat = atof(CONFIG_LATITUDE);
	const double lng = atof(CONFIG_LONGITUDE);
	
	// 1. get the day of the year (calculation is not required with the tm struct)
	const int N = now->tm_yday + 1;

	// 2. convert the longitude to hour value and calculate an approximate time
	const double lngHour = lng / 15;
	const double t = N + ((6 - lngHour) / 24);
	
	// 3. calculate the Sun's mean anomaly
	const double M = (0.9856 * t) - 3.289;
	
	// 4. calculate the Sun's true longitude and adjust angle to be between 0 and 360
	const double L = neg_fmod(M + (1.916 * sin(RAD(M))) + (0.020 * sin(RAD(2 * M))) + 282.634, 360.0);
	
	// 5a. calculate the Sun's right ascension and adjust angle to be between 0 and 360
	double RA = neg_fmod(DEG(atan(0.91764 * tan(RAD(L)))), 360.0);
	
	// 5b. right ascension value needs to be in the same quadrant as L
	const uint Lquadrant = ((uint) (L/90)) * 90;
	const uint RAquadrant = ((uint) (RA / 90)) * 90;
	RA += Lquadrant - RAquadrant;

	// 5c. right ascension value needs to be converted into hours
	RA /= 15;
	
	// 6. calculate the Sun's declination
	const double sinDec = 0.39782 * sin(RAD(L));
	const double cosDec = cos(asin(sinDec));

	// 7a. calculate the Sun's local hour angle
	const double cos_zenith = cos(RAD(ZENITH));
	const double radian_lat = RAD(lat);
	const double sin_lat = sin(radian_lat);
	const double cos_lat = cos(radian_lat);	
	const double cosH = (cos_zenith - (sinDec * sin_lat)) / (cosDec * cos_lat);

	// 7b. finish calculating H and convert into hours
	const double H = (360 - DEG(acos(cosH))) / 15;

	// 8. calculate local mean time of rising/setting
	const double T = H + RA - (0.06571 * t) - 6.622;

	// 9. adjust back to UTC
	const double UT = neg_fmod(T - lngHour, 24);

	// Convert the calculated time to time_t and save it
	struct tm sunrise = *now;
	sunrise.tm_hour = (int) UT;
	sunrise.tm_min = (int) (fmod(UT, 1.0) * 60);
	sunrise.tm_sec = 0;
	return mktime(&sunrise) + local_offset;
}

time_t calculate_sunset(const struct tm *now, int local_offset) {
	const double lat = atof(CONFIG_LATITUDE);
	const double lng = atof(CONFIG_LONGITUDE);
	
	// 1. get the day of the year (calculation is not required with the tm struct)
	const int N = now->tm_yday + 1;

	// 2. convert the longitude to hour value and calculate an approximate time
	const double lngHour = lng / 15;
	const double t = N + ((18 - lngHour) / 24);

	// 3. calculate the Sun's mean anomaly
	const double M = (0.9856 * t) - 3.289;

	// 4. calculate the Sun's true longitude and adjust angle to be between 0 and 360
	const double L = neg_fmod(M + (1.916 * sin(RAD(M))) + (0.020 * sin(RAD(2 * M))) + 282.634, 360.0);

	// 5a. calculate the Sun's right ascension and adjust angle to be between 0 and 360
	double RA = neg_fmod(DEG(atan(0.91764 * tan(RAD(L)))), 360.0);

	// 5b. right ascension value needs to be in the same quadrant as L
	const double Lquadrant = (double) (((uint) (L/90)) * 90);
	const double RAquadrant = (double) (((uint) (RA / 90)) * 90);
	RA += Lquadrant - RAquadrant;

	// 5c. right ascension value needs to be converted into hours
	RA /= 15;

	// 6. calculate the Sun's declination
	const double sinDec = 0.39782 * sin(RAD(L));
	const double cosDec = cos(asin(sinDec));

	// 7a. calculate the Sun's local hour angle
	const double cos_zenith = cos(RAD(ZENITH));
	const double radian_lat = RAD(lat);
	const double sin_lat = sin(radian_lat);
	const double cos_lat = cos(radian_lat);	
	const double cosH = (cos_zenith - (sinDec * sin_lat)) / (cosDec * cos_lat);

	// 7b. finish calculating H and convert into hours
	const double H = DEG(acos(cosH)) / 15;

	// 8. calculate local mean time of rising/setting
	const double T = H + RA - (0.06571 * t) - 6.622;

	// 9. adjust back to UTC
	const double UT = neg_fmod(T - lngHour, 24);

	// Convert the calculated time to time_t and save it
	struct tm sunset = *now;
	sunset.tm_hour = (int) UT;
	sunset.tm_min = (int) (fmod(UT, 1.0) * 60);
	sunset.tm_sec = 0;
	return mktime(&sunset) + local_offset;
}