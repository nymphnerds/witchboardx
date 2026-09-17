#define main cleanTestMain
#include "WitchboardCleanTest.cpp"
#undef main

struct TimingFixture
{
    static constexpr int mainBus = kNT_numInputBusses + 1;
    static constexpr int sendBus = kNT_numInputBusses + kNT_numOutputBusses + 1;
    static constexpr int returnBus = sendBus + 1;
    _NT_algorithmRequirements req = {};
    _NT_algorithmMemoryPtrs memory;
    WitchboardAlgorithm* alg;
    std::vector<int16_t> v;
    TimingFixture()
    {
        const int32_t specs[] = {2};
        calculateRequirements(req, specs);
        memory = allocateMemory(req);
        alg = static_cast<WitchboardAlgorithm*>(constructWitchboard(memory, req, specs));
        v.resize(req.numParameters);
        for (unsigned i = 0; i < req.numParameters; ++i) v[i] = alg->parameters[i].def;
        alg->v = v.data(); alg->vIncludingCommon = v.data();
        v[kParamFadeMs] = 0;
        v[kParamMainL] = mainBus; v[kParamMainR] = mainBus + 1;
        v[routeParam(0,kRouteOutputL)] = sendBus;
        v[routeParam(0,kRouteReturnL)] = returnBus;
        v[channelBase(0)+kChannelInputL] = 1;
        v[channelBase(0)+kChannelInsert1] = 1;
        v[channelBase(1)+kChannelInputL] = 2;
    }
    ~TimingFixture() { freeMemory(memory); }
    float sample(float inserted, float dry, float returned, float key = 0.0f)
    {
        std::vector<float> buses(kNT_lastBus * 4, 0.0f);
        busSample(buses,1,0) = inserted;
        busSample(buses,2,0) = dry;
        fillBus(buses,3,key);
        busSample(buses,returnBus,0) = returned;
        step(alg,buses.data(),1);
        return busSample(buses,mainBus,0);
    }
};

void testRouteEditorAndPreset()
{
    TimingFixture f;
    assert(f.alg->parameterPages->numPages == 8);
    assert(f.alg->parameterPages->pages[2].numParams == 4);
    const auto& latency = f.alg->parameterPages->pages[7];
    assert(strcmp(latency.name, "Latency") == 0 && latency.numParams == 4);
    assert(latency.params[0] == kParamSidechainLookahead);
    assert(latency.params[1] == kParamBypassOffset);
    assert(latency.params[2] == f.alg->insertSelectParam());
    assert(latency.params[3] == f.alg->insertLatencyParam());
    assert(f.alg->parameters[f.alg->insertLatencyParam()].min == 0);
    assert(f.alg->parameters[f.alg->insertLatencyParam()].max == 200);
    f.v[f.alg->insertLatencyParam()] = 200;
    parameterChanged(f.alg, f.alg->insertLatencyParam());
    assert(f.alg->insertLatencies[0] == 200);
    f.v[f.alg->insertSelectParam()] = 2;
    parameterChanged(f.alg, f.alg->insertSelectParam());
    assert(f.v[f.alg->insertLatencyParam()] == 0);
    f.v[f.alg->insertLatencyParam()] = 75;
    parameterChanged(f.alg, f.alg->insertLatencyParam());
    assert(f.alg->insertLatencies[1] == 75);
    JsonTape tape;
    { _NT_jsonStream stream(&tape); stream.openObject(); serialise(f.alg,stream); stream.closeObject(); }
    TimingFixture loaded;
    loaded.v = f.v; loaded.alg->v = loaded.v.data(); loaded.alg->vIncludingCommon = loaded.v.data();
    { _NT_jsonParse parse(&tape,0); assert(deserialise(loaded.alg,parse)); }
    loaded.sample(0,0,0);
    assert(loaded.alg->insertLatencies[0] == 200);
    assert(loaded.alg->insertLatencies[1] == 75);
    JsonTape legacy;
    { _NT_jsonStream stream(&legacy); stream.openObject(); stream.closeObject(); }
    { _NT_jsonParse parse(&legacy,0); assert(deserialise(loaded.alg,parse)); }
    loaded.sample(0,0,0);
    for (int route = 0; route < kNumRoutes; ++route) assert(loaded.alg->insertLatencies[route] == 0);
}

void testSingleAndSharedInsertTiming()
{
    TimingFixture f;
    const int latency = millisecondsToSamples(1.0f, NT_globals.sampleRate);
    f.v[f.alg->insertLatencyParam()] = 10; // 1 ms in tenths of a millisecond
    parameterChanged(f.alg, f.alg->insertLatencyParam());
    for (int i = 0; i < (latency + millisecondsToSamples(5.0f, NT_globals.sampleRate) + 8)/4; ++i) f.sample(0,0,0);
    for (int i = 0; i < latency/4; ++i)
    {
        float out = f.sample(i == 0 ? 1.0f : 0.0f,
            i == 0 ? 2.0f : 0.0f, 0.0f);
        assertClose(out,0.0f);
    }
    float aligned = f.sample(0,0,10.0f);
    assertClose(aligned,12.0f);
    assertClose(f.sample(0,0,0),0.0f);

    TimingFixture shared;
    shared.v[channelBase(1)+kChannelInsert1] = 1;
    shared.v[shared.alg->insertLatencyParam()] = 10;
    parameterChanged(shared.alg, shared.alg->insertLatencyParam());
    for (int i = 0; i < (latency + millisecondsToSamples(5.0f, NT_globals.sampleRate) + 8)/4; ++i) shared.sample(0,0,0);
    assertClose(shared.sample(1,2,10),10.0f); // one physical return
}

void testBypassInsertTiming()
{
    TimingFixture f;
    f.v[kParamMasterMode] = kMasterSum;
    f.v[channelBase(0)+kChannelOutputPath] = kOutputPathBypass;
    f.v[channelBase(1)+kChannelOutputPath] = kOutputPathBypass;
    f.v[f.alg->insertLatencyParam()] = 10;
    parameterChanged(f.alg, f.alg->insertLatencyParam());
    const int latency = millisecondsToSamples(1.0f, NT_globals.sampleRate);
    for (int i = 0; i < (latency + millisecondsToSamples(5.0f, NT_globals.sampleRate) + 8)/4; ++i)
        f.sample(0,0,0);
    for (int i = 0; i < latency/4; ++i)
        assertClose(f.sample(i == 0 ? 1.0f : 0.0f,
            i == 0 ? 2.0f : 0.0f,0),0.0f);
    assertClose(f.sample(0,0,10),12.0f);
}

void testSidechainKeyFollowsInsertTiming()
{
    TimingFixture f;
    f.v[kParamSidechainMode] = 1;
    f.v[kParamSidechainKeyInput] = 3;
    f.v[kParamSidechainLookahead] = 0;
    f.v[f.alg->insertLatencyParam()] = 10;
    parameterChanged(f.alg, f.alg->insertLatencyParam());
    const int latencyBlocks = millisecondsToSamples(1.0f, NT_globals.sampleRate) / 4;
    const int warm = (millisecondsToSamples(6.0f, NT_globals.sampleRate) + 8) / 4;
    for (int i = 0; i < warm; ++i) f.sample(0,0,0);
    f.sample(0,0,0,1.0f);
    assert(!f.alg->sidechain.keyHigh);
    for (int i = 1; i < latencyBlocks; ++i)
    {
        f.sample(0,0,0);
        assert(!f.alg->sidechain.keyHigh);
    }
    f.sample(0,0,0);
    assert(f.alg->sidechain.keyHigh);
}

void testLateSendOnDeferredInsertReturn()
{
    TimingFixture f;
    const int fx4Output = TimingFixture::sendBus + 2;
    f.v[fxParam(3, 0)] = fx4Output;
    f.v[channelBase(0) + kChannelSendSelect] = 3;
    f.v[channelBase(0) + kChannelSendAmount] = 25;
    f.v[f.alg->insertLatencyParam()] = 10;
    parameterChanged(f.alg, f.alg->insertLatencyParam());
    for (int i = 0; i < (millisecondsToSamples(6.0f, NT_globals.sampleRate) + 8) / 4; ++i)
        f.sample(0, 0, 0);
    std::vector<float> buses(kNT_lastBus * 4, 0.0f);
    busSample(buses, TimingFixture::returnBus, 0) = 10.0f;
    step(f.alg, buses.data(), 1);
    assertClose(busSample(buses, fx4Output, 0), 5.0f);
}

void testSharedReturnUsesLargestSendAfterSettling()
{
    TimingFixture f;
    const int fx4Output = TimingFixture::sendBus + 2;
    f.v[fxParam(3, 0)] = fx4Output;
    f.v[channelBase(1) + kChannelInsert1] = 1;
    f.v[channelBase(0) + kChannelSendSelect] = 3;
    f.v[channelBase(0) + kChannelSendAmount] = 25;
    f.v[channelBase(1) + kChannelSendSelect] = 3;
    f.v[channelBase(1) + kChannelSendAmount] = 50;
    f.v[f.alg->insertLatencyParam()] = 10;
    parameterChanged(f.alg, f.alg->insertLatencyParam());
    for (int i = 0; i < (millisecondsToSamples(6.0f, NT_globals.sampleRate) + 8) / 4; ++i)
        f.sample(0, 0, 0);
    std::vector<float> buses(kNT_lastBus * 4, 0.0f);
    busSample(buses, TimingFixture::returnBus, 0) = 10.0f;
    step(f.alg, buses.data(), 1);
    assertClose(busSample(buses, fx4Output, 0), 10.0f);
    f.v[channelBase(1) + kChannelSendAmount] = 0;
    parameterChanged(f.alg, channelBase(1) + kChannelSendAmount);
    std::fill(buses.begin(), buses.end(), 0.0f);
    busSample(buses, TimingFixture::returnBus, 0) = 10.0f;
    step(f.alg, buses.data(), 1);
    assertClose(busSample(buses, fx4Output, 0), 5.0f);
}

int main()
{
    testRouteEditorAndPreset();
    testSingleAndSharedInsertTiming();
    testBypassInsertTiming();
    testSidechainKeyFollowsInsertTiming();
    testLateSendOnDeferredInsertReturn();
    testSharedReturnUsesLargestSendAfterSettling();
    printf("PASS: insert route editor, preset migration, single/shared returns, Bypass and SC key timing at %d Hz\n", NT_globals.sampleRate);
}
