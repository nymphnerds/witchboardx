// Reuse the NT host stubs and routing helpers, without running the old suite here.
#define main cleanTestMain
#include "WitchboardCleanTest.cpp"
#undef main
#include <complex>

static double magnitude(const EqCoefficients& c, double hz, double rate)
{
	const std::complex<double> z = std::polar(1.0, -2*kEqPi*hz/rate);
	return std::abs((c.b0 + c.b1*z + c.b2*z*z)/(1.0 + c.a1*z + c.a2*z*z));
}

// Independent host reference: the Signalsmith oneSided equations, normal libm.
static EqCoefficients reference(double hz, double db, double q, double rate)
{
	const double w = 2*kEqPi*hz/rate;
	double inv2Q = 0.5/q;
	const double lower = sqrt(1 + inv2Q*inv2Q) - inv2Q;
	const double ratio = tan(w*0.5*lower)/tan(w*0.5);
	inv2Q = 0.5/ratio - 0.5*ratio;
	const double alpha = sin(w)*inv2Q, a = pow(10, db*0.025);
	const double scale = 1/(1 + alpha/a);
	return {(1 + alpha*a)*scale, -2*cos(w)*scale, (1 - alpha*a)*scale,
		-2*cos(w)*scale, (1 - alpha/a)*scale};
}

static double measuredGain(const EqCoefficients& c, double hz, double rate)
{
	EqState state = {};
	double inputEnergy = 0, outputEnergy = 0;
	for (int i = 0; i < static_cast<int>(rate); ++i)
	{
		const double x = sin(2*kEqPi*hz*i/rate);
		const double y = processEqBiquad(c, state, x);
		if (i >= rate/2) { inputEnergy += x*x; outputEnergy += y*y; }
	}
	return 10*log10(outputEnergy/inputEnergy);
}

static void testRouting(float rate)
{
	const int frames = 48;
	for (int band = 0; band < 3; ++band)
	for (int mode = kMasterSplit; mode <= kMasterInsert; ++mode)
	for (int filter = 0; filter < 2; ++filter)
	for (int sidechain = 0; sidechain < 2; ++sidechain)
	{
		const int32_t specs[] = {2};
		_NT_algorithmRequirements requirements = {};
		calculateRequirements(requirements,specs);
		_NT_algorithmMemoryPtrs memory = allocateMemory(requirements);
		_NT_algorithm* algorithm = constructWitchboard(memory,requirements,specs);
		std::vector<int16_t> values(requirements.numParameters);
		for (unsigned i = 0; i < requirements.numParameters; ++i) values[i] = algorithm->parameters[i].def;
		values[channelBase(0)+kChannelInputL] = 1;
		values[channelBase(1)+kChannelInputL] = 2;
		values[channelBase(1)+kChannelOutputPath] = kOutputPathBypass;
		values[kParamMainL] = 13; values[kParamMainR] = 14;
		values[kParamBypassL] = 15; values[kParamBypassR] = 16;
		values[kParamMasterSendL] = 17; values[kParamMasterSendR] = 18;
		values[kParamMasterReturnL] = 40; values[kParamMasterReturnR] = 41;
		values[kParamMasterMode] = mode;
		values[kParamEq1Freq+band*3] = 566;
		values[kParamEq1Gain+band*3] = 6;
		values[kParamSidechainMode] = sidechain;
		values[kParamSidechainKeyMode] = kKeyGate;
		values[kParamSidechainKeyInput] = 39;
		values[kParamSidechainDepth] = 50;
		values[kParamSidechainMakeup] = 0;
		values[kParamMasterFilterEnable] = filter;
		values[kParamMasterFilterSweep] = -50;
		algorithm->v = values.data(); algorithm->vIncludingCommon = values.data();
		EqState referenceState = {};
		const EqCoefficients ref = reference(eqFrequency(566,rate),6,eqQ(358),rate);
		MasterFilterRuntime filterState = {};
		const SvfCoefficients lp = makeFilterCoefficients(kFilterLowpass,
			hpCutoffPercentToHz(50,rate),filterQFromPercent(0));
		const SvfCoefficients unused = {};
		for (int block = 0; block < 100; ++block)
		{
			std::vector<float> buses(kNT_lastBus*frames,0);
			for (int i = 0; i < frames; ++i)
			{
				const float wave = sin(2*kEqPi*997*(block*frames+i)/rate);
				buses[i] = 0.01f*wave; buses[frames+i] = 0.02f*wave;
				buses[38*frames+i] = 5;
				buses[39*frames+i] = 0.037f*wave;
				buses[40*frames+i] = -0.021f*wave;
			}
			step(algorithm,buses.data(),frames/4);
			for (int i = 0; i < frames; ++i)
			{
				float expected = buses[i]*(sidechain ? 0.5f : 1.0f);
				if (mode != kMasterSplit) expected += buses[frames+i];
				float expectedR = expected;
				if (filter) processMasterFilter(filterState,expected,expectedR,true,false,lp,unused);
				expected = processEqBiquad(ref,referenceState,expected);
				if (mode == kMasterInsert)
				{
					assert(fabsf(buses[16*frames+i]-expected) < 0.000001f);
					assert(buses[12*frames+i] == buses[39*frames+i]);
					assert(buses[13*frames+i] == buses[40*frames+i]);
				}
				else assert(fabsf(buses[12*frames+i]-expected) < 0.000001f);
				if (mode == kMasterSplit) assert(buses[14*frames+i] == buses[frames+i]);
			}
		}
		freeMemory(memory);
	}
}

int main()
{
	const float rate = NT_globals.sampleRate;
	testRouting(rate);
	assert(fabsf(eqFrequency(1000, rate) - std::min(20000.0f, rate*0.45f)) < 0.01f);
	assert(fabsf(eqFrequency(0, rate) - 20) < 0.001f);
	assert(fabsf(eqQ(0) - 0.25f) < 0.00001f && fabsf(eqQ(1000) - 12) < 0.00001f);
	assert(fabsf(eqFrequency(259, rate) - 120) < 1);
	assert(fabsf(eqFrequency(566, rate) - 1000) < 3);
	assert(fabsf(eqFrequency(867, rate) - 8000) < 30);

	for (float q : {0.25f, 1.0f, 12.0f})
	for (float hz : {20.0f, 120.0f, 1000.0f, 8000.0f, rate*0.45f})
	for (float db : {-18.0f, -12.0f, 0.0f, 6.0f, 18.0f})
	{
		const EqCoefficients c = makeEqCoefficients(hz, db, q, rate);
		const EqCoefficients ref = reference(hz, db, q, rate);
		// Jury stability and frequency-response agreement, including low end/high Q.
		assert(fabs(c.a2) < 1 && 1+c.a1+c.a2 > 0 && 1-c.a1+c.a2 > 0);
		for (int i = 0; i <= 100; ++i)
		{
			const double probe = 10*pow(rate*0.049, i/100.0);
			const double error = fabs(20*log10(magnitude(c,probe,rate)/magnitude(ref,probe,rate)));
			if (error >= 0.0001) fprintf(stderr,"response error %.9f dB hz %g db %g q %g probe %g\n",error,hz,db,q,probe);
			assert(error < 0.0001);
		}
		assert(fabs(20*log10(magnitude(c,hz,rate)) - db) < 0.0001);
		if (!db)
		{
			EqState state = {};
			for (int i = 0; i < 1000; ++i)
			{
				const double x = sin(i*0.123);
				assert(processEqBiquad(c,state,x) == x);
			}
		}
	}
	assert(fabs(measuredGain(makeEqCoefficients(1000,6,1,rate),1000,rate)-6) < 0.001);
	assert(fabs(measuredGain(makeEqCoefficients(1000,-12,1,rate),1000,rate)+12) < 0.001);
	assert(magnitude(makeEqCoefficients(1000,6,8,rate),700,rate)
		< magnitude(makeEqCoefficients(1000,6,0.5f,rate),700,rate));

	buildParameters();
	int16_t values[kMaxParams];
	for (int i = 0; i < kMaxParams; ++i) values[i] = parameterDefs[i].def;
	MasterEqRuntime eq = {};
	prepareMasterEq(eq,values,rate);
	float previous = 0, maximumJump = 0, maximumOutput = 0;
	uint32_t random = 0x712abc;
	for (int i = 0; i < static_cast<int>(rate)*8; ++i)
	{
		// Full-range jumps, including alternating boosts/cuts and returning flat.
		if (i % 193 == 0)
		{
			for (int band = 0; band < 3; ++band)
			{
				random = random*1664525u + 1013904223u;
				values[kParamEq1Freq+3*band] = random%1001;
				values[kParamEq1Gain+3*band] = (random>>12)%37-18;
				values[kParamEq1Q+3*band] = (random>>20)%1001;
			}
			prepareMasterEq(eq,values,rate);
		}
		advanceMasterEq(eq);
		float left = 0.01f*sin(2*kEqPi*997*i/rate), right = 0;
		processMasterEq(eq,left,right);
		assert(std::isfinite(left) && right == 0);
		maximumOutput = std::max(maximumOutput,fabsf(left));
		maximumJump = std::max(maximumJump,fabsf(left-previous));
		previous = left;
	}
	assert(maximumOutput < 0.5f && maximumJump < 0.1f);
	for (int band = 0; band < 3; ++band) values[kParamEq1Gain+3*band] = 0;
	prepareMasterEq(eq,values,rate);
	for (int i = 0; i < rate; ++i)
	{
		advanceMasterEq(eq);
		float left = 0.1f, right = -0.2f;
		processMasterEq(eq,left,right);
		if (i > rate/2) assert(left == 0.1f && right == -0.2f);
	}
	assert(!prepareMasterEq(eq,values,rate));

	char text[kNT_parameterStringSize] = {};
	assert(parameterString(NULL,kParamEq1Freq,0,text)>0 && strcmp(text,"20 Hz")==0);
	assert(parameterString(NULL,kParamEq1Q,0,text)>0 && strcmp(text,"0.25")==0);
	assert(parameterString(NULL,kParamEq1Q,1000,text)>0 && strcmp(text,"12.00")==0);
	printf("PASS: EQ at %.0f Hz: reference/flat/boost/cut/Q/stereo/live movement/clamp; peak %.5f, max step %.5f\n",
		rate, maximumOutput, maximumJump);
}
