// $Id$

#include "v473.h"
#include <cmath>
#include <taskLib.h>

#define M_PI 3.14159265358979323846

typedef float POINT[4];
typedef float const CPOINT[4];
typedef float MATRIX[4][4];
typedef float const CMATRIX[4][4];

static float clip(float val, float limit)
{
    return std::min(std::max(val, -limit), limit);
}

int v473_eye = 2;
int v473_delta = 300;
int v473_rotx = 3;
int v473_roty = 3;
int v473_rotz = 3;

static void project(CMATRIX m, CPOINT p, float b[2])
{
    POINT tmp;
    float const eye = v473_eye;

    for (size_t row = 0; row < 3; ++row)
	for (size_t col = 0; col < 4; ++col)
	    tmp[row] =
		m[row][0] * p[0] +
		m[row][1] * p[1] +
		m[row][2] * p[2] +
		m[row][3];

    b[0] = clip(tmp[0] * eye / (tmp[2] + eye), 2.f);
    b[1] = clip(tmp[1] * eye / (tmp[2] + eye), 2.f);
}

void dumpMatrix(CMATRIX m)
{
    for (size_t ii = 0; ii < 4; ++ii)
	printf("    | %6.3f %6.3f %6.3f %6.3f |\n",
	       m[ii][0], m[ii][1], m[ii][2], m[ii][3]);
}

static void product(CMATRIX const p, MATRIX m)
{
    MATRIX tmp;

    for (size_t row = 0; row < 4; ++row)
	for (size_t col = 0; col < 4; ++col)
	    tmp[row][col] =
		m[row][0] * p[0][col] +
		m[row][1] * p[1][col] +
		m[row][2] * p[2][col] +
		m[row][3] * p[3][col];
    for (size_t row = 0; row < 4; ++row)
	for (size_t col = 0; col < 4; ++col)
	    m[row][col] = tmp[row][col];
}

static void identity(MATRIX m)
{
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;
    m[0][1] = m[0][2] = m[0][3] = m[1][0] = m[1][2] = m[1][3] = m[2][0] =
	m[2][1] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.0;
}

static void translate(MATRIX m, float const dx, float const dy,
		      float const dz)
{
    m[3][0] += dx;
    m[3][1] += dy;
    m[3][2] += dz;
}

void rotateX(MATRIX m, float const a)
{
    float const r = (2.0 * M_PI / 360.0) * a;
    float const s = sin(r);
    float const c = cos(r);

    CMATRIX mx = {
	{1., 0., 0., 0.},
	{0.,  c,  s, 0.},
	{0., -s,  c, 0.},
	{0., 0., 0., 1.}
    };

    product(mx, m);
}

void rotateY(MATRIX m, float const a)
{
    float const r = (2.0 * M_PI / 360.0) * a;
    float const s = sin(r);
    float const c = cos(r);

    CMATRIX my = {
	{ c, 0., -s, 0.},
	{0., 1., 0., 0.},
	{ s, 0.,  c, 0.},
	{0., 0., 0., 1.}
    };

    product(my, m);
}

void rotateZ(MATRIX m, float const a)
{
    float const r = (2.0 * M_PI / 360.0) * a;
    float const s = sin(r);
    float const c = cos(r);

    CMATRIX mz = {
	{ c, -s, 0., 0.},
	{ s,  c, 0., 0.},
	{0., 0., 1., 0.},
	{0., 0., 0., 1.}
    };

    product(mz, m);
}

STATUS v473_cube(V473::HANDLE const hw)
{
    int xa = 0;
    int ya = 0;
    int za = 0;
    int ramp = 0;

    try {
	{
	    // Setup the hardware

	    uint16_t data;
	    vwpp::Lock lock(hw->mutex);

	    hw->waveformEnable(lock, 0, false);
	    hw->waveformEnable(lock, 1, false);

	    data = 1;
	    hw->setRampMap(lock, 0, 0, &data, 1);
	    hw->setRampMap(lock, 1, 0, &data, 1);
	    data = 2;
	    hw->setRampMap(lock, 0, 1, &data, 1);
	    hw->setRampMap(lock, 1, 1, &data, 1);

	    // Set the scale factor to 1.0.

	    data = 128;
	    hw->setScaleFactors(lock, 0, 0, &data, 1);
	    hw->setScaleFactors(lock, 1, 0, &data, 1);
	    data = 1;
	    hw->setScaleFactorMap(lock, 0, 0, &data, 1);
	    hw->setScaleFactorMap(lock, 0, 1, &data, 1);
	    hw->setScaleFactorMap(lock, 1, 0, &data, 1);
	    hw->setScaleFactorMap(lock, 1, 1, &data, 1);

	    // Set the offset

	    data = 0;
	    hw->setOffsetMap(lock, 0, 0, &data, 1);
	    hw->setOffsetMap(lock, 0, 1, &data, 1);
	    hw->setOffsetMap(lock, 1, 0, &data, 1);
	    hw->setOffsetMap(lock, 1, 1, &data, 1);
	    hw->tclkTrigEnable(lock, true);

	    uint8_t const event = 0x0f;

	    hw->setTriggerMap(lock, 0, &event, 1);

	    // Enable channels 0 and 1.

	    hw->waveformEnable(lock, 0, true);
	    hw->waveformEnable(lock, 1, true);
	}

	do {
	    static size_t const path[] = {
		0, 1, 2, 3, 7, 6, 5, 4, 0, 3, 2, 6, 7, 4, 5, 1, 0
	    };

	    // Update rotation

	    xa = (xa + v473_rotx + 360) % 360;
	    ya = (ya + v473_roty + 360) % 360;
	    za = (za + v473_rotz + 360) % 360;

	    // Compute transform matrix

	    MATRIX m;

	    identity(m);
	    rotateX(m, xa);
	    rotateY(m, ya);
	    rotateZ(m, za);
	    translate(m, 0., 0., 2.);

	    // Translate each point and save result into ramp table
	    // buffer.

	    uint16_t data[2][2 * (sizeof(path) / sizeof(*path) + 1)];

	    for (size_t ii = 0; ii < sizeof(path) / sizeof(*path); ++ii) {
		static POINT const point[] = {
		    { -0.5,  0.5, -0.5, 1. },
		    { -0.5,  0.5,  0.5, 1. },
		    {  0.5,  0.5,  0.5, 1. },
		    {  0.5,  0.5, -0.5, 1. },
		    { -0.5, -0.5, -0.5, 1. },
		    { -0.5, -0.5,  0.5, 1. },
		    {  0.5, -0.5,  0.5, 1. },
		    {  0.5, -0.5, -0.5, 1. }
		};
		float b[2];

		project(m, point[path[ii]], b);
		data[0][ii * 2] = uint16_t(b[0] * 16000.);
		data[1][ii * 2] = uint16_t(b[1] * 16000.);
		data[0][ii * 2 + 1] = data[1][ii * 2 + 1] = v473_delta;

		data[0][(ii + 1) * 2] = uint16_t(b[0] * 16000.);
		data[1][(ii + 1) * 2] = uint16_t(b[1] * 16000.);
		data[0][(ii + 1) * 2 + 1] = data[1][(ii + 1) * 2 + 1] = 0;
	    }

	    vwpp::Lock lock(hw->mutex);
	    {
		uint8_t const unevent = 0xfe;

		hw->setTriggerMap(lock, !ramp, &unevent, 1);

		hw->setRamp(lock, 0, ramp + 1, 0, data[0], 2 * (sizeof(path) / sizeof(*path) + 1));
		hw->setRamp(lock, 1, ramp + 1, 0, data[1], 2 * (sizeof(path) / sizeof(*path) + 1));

		// Hand the $0f event to the interrupt assigned to the
		// next ramp.

		uint8_t const event = 0x0f;

		hw->setTriggerMap(lock, ramp, &event, 1);
	    }

	    // Wait for the ramp to start to play. Then we can switch
	    // to updating the other ramp.

	    while (hw->getActiveInterruptLevel(lock) == ramp)
		taskDelay(1);
	    ramp = !ramp;
	} while (true);
    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}
