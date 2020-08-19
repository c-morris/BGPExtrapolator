#include "ASes/EZAS.h"

EZAS::EZAS(uint32_t asn) : BaseAS<EZAnnouncement>(asn, false, NULL) { }
EZAS::EZAS() : EZAS(0) { }
EZAS::~EZAS() { }