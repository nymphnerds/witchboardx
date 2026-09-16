#define main cleanTestMain
#include "WitchboardCleanTest.cpp"
#undef main

struct SendFixture
{
    _NT_algorithmRequirements req = {};
    _NT_algorithmMemoryPtrs memory;
    WitchboardAlgorithm* alg;
    std::vector<int16_t> values;
    SendFixture()
    {
        const int32_t specs[] = {10};
        calculateRequirements(req, specs);
        memory = allocateMemory(req);
        alg = static_cast<WitchboardAlgorithm*>(constructWitchboard(memory, req, specs));
        values.resize(req.numParameters);
        for (unsigned i = 0; i < req.numParameters; ++i) values[i] = alg->parameters[i].def;
        alg->v = values.data();
        alg->vIncludingCommon = values.data();
    }
    ~SendFixture() { freeMemory(memory); }
    void set(int p, int v) { values[p] = v; parameterChanged(alg, p); }
    void select(int ch, int fx) { set(channelBase(ch) + kChannelSendSelect, fx); }
    void amount(int ch, int v) { set(channelBase(ch) + kChannelSendAmount, v); }
};

void testFourStateSendSelector()
{
    SendFixture f;
    const int parameter = channelBase(0) + kChannelSendSelect;
    assert(f.alg->parameters[parameter].min == 0);
    assert(f.alg->parameters[parameter].max == 4);
    const int cc[] = {0, 42, 85, 127};
    for (int send = 0; send < 4; ++send)
    {
        // NT's integer CC scaling, with mapping endpoints 0..4.
        const int value = cc[send] * 4 / 127;
        f.select(0, value);
        assert(f.alg->sends[0].selected == send);
        char label[kNT_parameterStringSize] = {};
        parameterString(f.alg, parameter, value, label);
        assert(strcmp(label, defaultFxNames[send]) == 0);
        f.amount(0, 10 + send * 20);
    }
    f.select(0, 3);
    assert(f.alg->sends[0].selected == 3);
    assert(f.values[channelBase(0) + kChannelSendAmount] == 70);
    f.select(0, 4);
    assert(f.alg->sends[0].selected == 3);
    assert(f.values[channelBase(0) + kChannelSendAmount] == 70);
}

void testSendEditorAndMidi()
{
    SendFixture f;
    const int ch = 9, base = channelBase(ch);
    for (int fx = 0; fx < 4; ++fx) { f.select(ch, fx); f.amount(ch, 10 + fx * 20); }
    for (int fx = 0; fx < 4; ++fx)
    {
        f.select(ch, fx);
        assert(f.values[base + kChannelSendAmount] == 10 + fx * 20);
        assert(f.alg->sends[ch].levels[fx] == 10 + fx * 20);
        f.alg->sends[ch].midi[fx] = {16, static_cast<int16_t>(40+fx), 0, 100, 0, -1, false};
    }
    f.select(ch, 2);
    for (int fx = 0; fx < 4; ++fx) midiMessage(f.alg, 0xbf, 40+fx, 127);
    for (int fx = 0; fx < 4; ++fx) assert(f.alg->sends[ch].levels[fx] == 100);
    assert(f.values[base + kChannelSendSelect] == 2);
    assert(f.values[base + kChannelSendAmount] == 100);
    assert(f.alg->sends[0].levels[0] == 0);
    midiMessage(f.alg, 0xbe, 42, 0); // wrong MIDI channel
    midiMessage(f.alg, 0x9f, 42, 0); // note, not CC
    assert(f.alg->sends[ch].levels[2] == 100);
    midiMessage(f.alg, 0xbf, 42, 0);
    assert(f.values[base + kChannelSendAmount] == 0);

    // A shared fader catches the selected send without altering the other three.
    auto& shared = f.alg->sends[ch].midi[4];
    shared = {16, 50, 0, 100, 1, -1, false};
    f.amount(ch, 50);
    midiMessage(f.alg, 0xbf, 50, 0);
    assert(f.alg->sends[ch].levels[2] == 50);
    midiMessage(f.alg, 0xbf, 50, 64);
    assert(shared.caught);
    midiMessage(f.alg, 0xbf, 50, 100);
    assert(f.alg->sends[ch].levels[2] == 78);
    f.select(ch, 1);
    assert(!shared.caught && f.values[base + kChannelSendAmount] == 100);
    midiMessage(f.alg, 0xbf, 50, 101);
    assert(f.alg->sends[ch].levels[1] == 100);
    midiMessage(f.alg, 0xbf, 50, 127);
    midiMessage(f.alg, 0xbf, 50, 64);
    assert(f.alg->sends[ch].levels[1] == 50);
    assert(f.alg->sends[ch].levels[2] == 78);
    // A fixed fader moving the selected send re-arms shared pickup.
    midiMessage(f.alg, 0xbf, 41, 0);
    assert(!shared.caught && f.values[base + kChannelSendAmount] == 0);
    midiMessage(f.alg, 0xbf, 50, 65);
    assert(f.alg->sends[ch].levels[1] == 0);
    f.alg->sends[ch].midi[0] = {16, 40, 100, 0, 0, -1, false};
    midiMessage(f.alg, 0xbf, 40, 127);
    assert(f.alg->sends[ch].levels[0] == 0);
}

void testFourSendAudio()
{
    SendFixture f;
    f.values[kParamFadeMs] = 0;
    f.values[kParamMainL] = 13; f.values[kParamMainR] = 14;
    f.values[kParamBypassL] = 15; f.values[kParamBypassR] = 16;
    f.values[channelBase(9) + kChannelInputL] = 1;
    f.values[channelBase(9) + kChannelInputR] = 2;
    for (int fx = 0; fx < 4; ++fx)
    {
        f.values[fxParam(fx,0)] = 17+fx*2;
        f.values[fxParam(fx,1)] = 18+fx*2;
        f.values[fxParam(fx,3)] = 3+fx*2;
        f.values[fxParam(fx,4)] = 4+fx*2;
        f.values[fxParam(fx,6)] = fx % 2;
        f.select(9, fx); f.amount(9, 50);
    }
    std::vector<float> buses(kNT_lastBus*4, 0);
    fillBus(buses,1,0.25f); fillBus(buses,2,-0.5f);
    for (int fx = 0; fx < 4; ++fx)
    { fillBus(buses,3+fx*2,fx+1); fillBus(buses,4+fx*2,-(fx+1)); }
    step(f.alg,buses.data(),1);
    for (int fx = 0; fx < 4; ++fx)
    { assertBus(buses,17+fx*2,0.25f); assertBus(buses,18+fx*2,-0.5f); }
    assertBus(buses,13,4.25f); assertBus(buses,14,-4.5f);
    assertBus(buses,15,6); assertBus(buses,16,-6);
    // Full mix on FX4 removes dry only; FX1..3 stay active.
    f.amount(9,100);
    std::fill(buses.begin(),buses.end(),0);
    fillBus(buses,1,0.25f); fillBus(buses,2,-0.5f);
    step(f.alg,buses.data(),1);
    assertBus(buses,13,0);
    for (int fx = 0; fx < 4; ++fx) assertBus(buses,17+fx*2,0.25f);
    // Mono output folds stereo, and mono return ignores its R assignment.
    f.values[fxParam(3,2)] = kWidthMono;
    f.values[fxParam(3,5)] = kWidthMono;
    std::fill(buses.begin(),buses.end(),0);
    fillBus(buses,1,0.25f); fillBus(buses,2,-0.5f);
    fillBus(buses,9,2); fillBus(buses,10,99);
    step(f.alg,buses.data(),1);
    assertBus(buses,23,-0.125f); assertBus(buses,24,0);
    assertBus(buses,15,2); assertBus(buses,16,2);
}

void testInactiveSendBecomesActiveDuringFade()
{
    SendFixture f;
    f.values[kParamFadeMs] = 1;
    f.values[kParamMainL] = 13;
    f.values[fxParam(3, 0)] = 17;
    f.values[channelBase(9) + kChannelInputL] = 1;
    f.select(9, 3);
    f.amount(9, 0);
    std::vector<float> buses(kNT_lastBus * 4, 0.0f);
    fillBus(buses, 1, 1.0f);
    step(f.alg, buses.data(), 1);
    f.amount(9, 100);
    std::fill(buses.begin(), buses.end(), 0.0f);
    fillBus(buses, 1, 1.0f);
    step(f.alg, buses.data(), 1);
    assert(busSample(buses, 17, 0) > 0.0f);
    for (int i = 0; i < 30; ++i)
    {
        std::fill(buses.begin(), buses.end(), 0.0f);
        fillBus(buses, 1, 1.0f);
        step(f.alg, buses.data(), 1);
    }
    assertBus(buses, 17, 1.0f);
    f.amount(9, 0);
    for (int i = 0; i < 30; ++i)
    {
        std::fill(buses.begin(), buses.end(), 0.0f);
        fillBus(buses, 1, 1.0f);
        step(f.alg, buses.data(), 1);
    }
    assertBus(buses, 17, 0.0f);
    assertBus(buses, 13, 1.0f);
}

void testSendSaveLoadAndPages()
{
    SendFixture source;
    bool seen[kMaxParams] = {};
    for (unsigned page = 0; page < source.alg->pages.numPages; ++page)
    {
        const auto& p = source.alg->pages.pages[page];
        for (int i = 0; i < p.numParams; ++i)
        { assert(p.params[i] < kMaxParams); assert(!seen[p.params[i]]); seen[p.params[i]] = true; }
    }
    for (bool b : seen) assert(b); // Every identity exactly once, including Switch fade.
    for (int ch = 0; ch < 10; ++ch)
    {
        for (int fx = 0; fx < 4; ++fx) { source.select(ch,fx); source.amount(ch,ch*7+fx*3); }
        source.alg->sends[ch].midi[4] = {static_cast<int16_t>(ch+1), 9, 0, 100, 1, -1, false};
        source.select(ch,ch%4);
    }
    JsonTape tape;
    {_NT_jsonStream stream(&tape); stream.openObject(); serialise(source.alg,stream); stream.closeObject();}
    for (bool notifyBefore : {false,true})
    {
        SendFixture loaded;
        loaded.values = source.values;
        loaded.alg->v = loaded.values.data();
        if (notifyBefore) for (int p = 0; p < kMaxParams; ++p) parameterChanged(loaded.alg,p);
        {_NT_jsonParse parse(&tape,0); assert(deserialise(loaded.alg,parse));}
        if (!notifyBefore) for (int p = 0; p < kMaxParams; ++p) parameterChanged(loaded.alg,p);
        stepOnce(loaded.alg,loaded.values);
        for (int ch = 0; ch < 10; ++ch)
        {
            for (int fx = 0; fx < 4; ++fx)
                assert(loaded.alg->sends[ch].levels[fx] == source.alg->sends[ch].levels[fx]);
            assert(loaded.alg->sends[ch].midi[4].channel == ch+1);
            assert(loaded.alg->sends[ch].midi[4].pickup == 1);
        }
        assert(loaded.values == source.values);
    }
}

int main()
{
	testFourStateSendSelector(); testSendEditorAndMidi(); testFourSendAudio();
	testInactiveSendBecomesActiveDuringFade(); testSendSaveLoadAndPages();
    printf("PASS: four simultaneous sends/returns, editor, independent/shared CCs, pickup, save/load, unique pages at %.0f Hz\n",double(NT_globals.sampleRate));
}
