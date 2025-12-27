# dimming-curves


Minimaler Usage-Snippet (Applikation)
#include "dim_curve.h"

uint16_t curve[DimCurve::TABLE_SIZE];

void setup() {
  DimCurve::gamma(curve, 2.2f);               // oder exponential / daliLog / ledHybrid / ...
  Light.configDimCurve(curve, DimCurve::TABLE_SIZE);
}


Die Library liefert immer uint16_t[256]



----


Typische Verwendung (optional, fail-safe)
uint16_t curve[DimCurve::TABLE_SIZE];

DimCurve::gamma(curve, 2.2f);

if (DimCurve::validate(curve)) {
  Light.configDimCurve(curve, DimCurve::TABLE_SIZE);
} else {
  // optional loggen, aber nichts tun -> esp-knx-led bleibt bei lookupTable
}