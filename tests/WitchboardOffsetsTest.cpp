#define main cleanTestMain
#include "WitchboardCleanTest.cpp"
#undef main

struct OffsetFixture
{
    _NT_algorithmRequirements req = {};
    _NT_algorithmMemoryPtrs memory;
    WitchboardAlgorithm* alg;
    std::vector<int16_t> v;
    OffsetFixture(int channels = 10)
    {
        const int32_t specs[] = {channels};
        calculateRequirements(req, specs);
        memory = allocateMemory(req);
        alg = static_cast<WitchboardAlgorithm*>(constructWitchboard(memory, req, specs));
        v.resize(req.numParameters);
        for (unsigned p = 0; p < req.numParameters; ++p) v[p] = alg->parameters[p].def;
        alg->v = v.data(); alg->vIncludingCommon = v.data();
        v[kParamFadeMs] = 0;
        v[kParamMainL] = 13; v[kParamMainR] = 14;
        v[kParamBypassL] = 15; v[kParamBypassR] = 16;
    }
    ~OffsetFixture() { freeMemory(memory); }
    void select(int ch) { v[alg->offsetChannelParam()] = ch; parameterChanged(alg, alg->offsetChannelParam()); }
    void amount(int value) { v[alg->offsetValueParam()] = value; parameterChanged(alg, alg->offsetValueParam()); }
    void offset(int ch, int value) { select(ch); amount(value); }
};

void testOffsetEditorAndRestore()
{
    OffsetFixture source;
    for (int ch = 1; ch <= 10; ++ch) source.offset(ch, -ch*30);
    for (int ch = 10; ch >= 1; --ch)
    {
        source.select(ch);
        assert(source.v[source.alg->offsetValueParam()] == -ch*30);
    }
    // Save immediately after a panel edit, without an audio block in between.
    JsonTape tape;
    { _NT_jsonStream stream(&tape); stream.openObject(); serialise(source.alg,stream); stream.closeObject(); }
    for (bool before : {false, true})
    {
        OffsetFixture loaded;
        loaded.v = source.v; loaded.alg->v = loaded.v.data();
        if (before) for (unsigned p = 0; p < loaded.req.numParameters; ++p) parameterChanged(loaded.alg,p);
        { _NT_jsonParse parse(&tape,0); assert(deserialise(loaded.alg,parse)); }
        if (!before) for (unsigned p = 0; p < loaded.req.numParameters; ++p) parameterChanged(loaded.alg,p);
        stepOnce(loaded.alg,loaded.v);
        assert(loaded.v == source.v);
        for (int ch = 1; ch <= 10; ++ch)
        {
            loaded.select(ch);
            assert(loaded.v[loaded.alg->offsetValueParam()] == -ch*30);
        }
    }
    // Older metadata has no offsets: no stale values survive a restore.
    JsonTape legacy;
    { _NT_jsonStream stream(&legacy); stream.openObject(); stream.closeObject(); }
    { _NT_jsonParse parse(&legacy,0); assert(deserialise(source.alg,parse)); }
    stepOnce(source.alg,source.v);
    for (int ch = 1; ch <= 10; ++ch) { source.select(ch); assert(source.v[source.alg->offsetValueParam()] == 0); }
    // Audio polling also handles writes without notifications, including CV.
    source.v[source.alg->offsetValueParam()] = -123;
    stepOnce(source.alg, source.v);
    assert(source.alg->channelOffsets[9] == -123);
    source.v[source.alg->offsetChannelParam()] = 1;
    stepOnce(source.alg, source.v);
    assert(source.v[source.alg->offsetValueParam()] == 0);
    assert(source.alg->channelOffsets[9] == -123);
    for (int invalid : {-301, 1})
    {
        JsonTape bad;
        { _NT_jsonStream stream(&bad); stream.openObject(); stream.addMemberName("witchboardChannelOffsets");
          stream.openArray(); for (int i=0;i<10;++i) stream.addNumber(invalid); stream.closeArray(); stream.closeObject(); }
        _NT_jsonParse parse(&bad,0); assert(!deserialise(source.alg,parse));
    }
}

void testOffsetParameterLayouts()
{
    OffsetFixture ten;
    for (int channels = 1; channels <= 10; ++channels)
    {
        OffsetFixture f(channels);
        assert(f.req.numParameters == unsigned(91 + channels*15));
        const int first = 89 + channels*15;
        assert(f.alg->parameters[first].max == channels);
        assert(f.alg->parameters[first+1].min == -300 && f.alg->parameters[first+1].max == 0);
        const auto& page = f.alg->pages.pages[5+channels];
        assert(strcmp(page.name,"Offset") == 0 && page.numParams == 2);
        assert(page.params[0] == first && page.params[1] == first+1);
        std::vector<bool> seen(f.req.numParameters,false);
        for (unsigned i=0;i<f.alg->pages.numPages;++i)
            for(int j=0;j<f.alg->pages.pages[i].numParams;++j)
            {
                int p=f.alg->pages.pages[i].params[j];
                assert(p<int(seen.size()) && !seen[p]); seen[p]=true;
            }
        for(bool b:seen) assert(b);
        const uintptr_t end = reinterpret_cast<uintptr_t>(f.memory.dram) + f.req.dram;
        assert(reinterpret_cast<uintptr_t>(f.alg->offsetKeyDelay.data + 2*kChannelDelayCapacity) <= end);
    }
    // Definitions must not be shared/mutated across different channel counts.
    assert(ten.alg->parameters[239].max == 10);
}

void testOffsetImpulseRouting()
{
    const int base = millisecondsToSamples(30, NT_globals.sampleRate);
    const int middle = base - millisecondsToSamples(12.3f, NT_globals.sampleRate);
    for (int mode : {kMasterSplit, kMasterSum, kMasterInsert})
    for (bool insert : {false,true})
    {
        OffsetFixture f(3);
        f.v[kParamMasterMode] = mode;
        f.v[kParamMasterSendL] = 19; f.v[kParamMasterSendR] = 20;
        for (int ch=0;ch<3;++ch)
        {
            f.v[channelBase(ch)+kChannelInputL] = 1+ch*2;
            f.v[channelBase(ch)+kChannelInputR] = 2+ch*2;
            f.v[channelBase(ch)+kChannelSendAmount] = 50;
        }
        f.v[channelBase(1)+kChannelOutputPath] = kOutputPathBypass;
        f.v[kParamFx1L] = 17; f.v[kParamFx1R] = 18;
        f.offset(1,-300); f.offset(2,-123);
        if (insert)
        {
            f.v[channelBase(1)+kChannelInsert1] = 1;
            f.v[routeParam(0,kRouteReturnL)] = 7; f.v[routeParam(0,kRouteReturnR)] = 8;
            f.v[routeParam(0,kRouteReturnWidth)] = kWidthStereo;
            f.v[routeParam(0,kRouteOutputL)] = 21; f.v[routeParam(0,kRouteOutputR)] = 22;
            f.v[routeParam(0,kRouteSendWidth)] = kWidthStereo;
        }
        for(int start=0;start<base+16;start+=4)
        {
            std::vector<float> buses(kNT_lastBus*4,0);
            if(start==0)
            {
                for(int ch=0;ch<3;++ch) { busSample(buses,1+ch*2,0)=ch+1; busSample(buses,2+ch*2,0)=-(ch+1); }
                busSample(buses,7,0)=2; busSample(buses,8,0)=-2;
                if(insert) { busSample(buses,3,0)=99; busSample(buses,4,0)=-99; }
            }
            step(f.alg,buses.data(),1);
            for(int n=0;n<4;++n)
            {
                int t=start+n;
                float main=(t==0?1:0)+(t==base?3:0), bypass=t==middle?2:0;
                int out=mode==kMasterInsert?19:13;
                assertClose(busSample(buses,out,n), main+(mode==kMasterSplit?0:bypass));
                assertClose(busSample(buses,out+1,n), -main-(mode==kMasterSplit?0:bypass));
                assertClose(busSample(buses,15,n),mode==kMasterSplit?bypass:0);
                assertClose(busSample(buses,16,n),mode==kMasterSplit?-bypass:0);
                assertClose(busSample(buses,17,n),main+bypass);
                assertClose(busSample(buses,18,n),-main-bypass);
                if(insert) assertClose(busSample(buses,21,n),t==0?99:0);
            }
        }
    }
}

void testOffsetSidechainAlignment()
{
    OffsetFixture reference(2), delayed(2);
    for(auto* f : {&reference,&delayed})
    {
        f->v[channelBase(0)+kChannelInputL]=1;
        f->v[kParamSidechainMode]=1;
        f->v[kParamSidechainKeyInput]=5;
    }
    // A silent channel establishes the compensation base; the trigger follows it.
    delayed.offset(2,-300);
    int base=millisecondsToSamples(30,NT_globals.sampleRate);
    std::vector<float> expected;
    for(int start=0;start<base+2048;start+=4)
    {
        for(auto* f : {&reference,&delayed})
        {
            std::vector<float> buses(kNT_lastBus*4,0);
            for(int n=0;n<4;++n)
            {
                int t=start+n;
                busSample(buses,1,n)=t>=64?1:0;
                busSample(buses,5,n)=t==64?5:0;
            }
            step(f->alg,buses.data(),1);
            for(int n=0;n<4;++n)
                if(f==&reference) expected.push_back(busSample(buses,13,n));
                else assertClose(busSample(buses,13,n),start+n<base?0:expected[start+n-base]);
        }
    }
}

void testSharedFinalInsertReturn()
{
    OffsetFixture f(3);
    f.v[channelBase(0)+kChannelInputL] = 1;
    f.v[channelBase(1)+kChannelInputL] = 2;
    f.v[channelBase(2)+kChannelInputL] = 3;
    f.v[channelBase(0)+kChannelInsert1] = 1;
    f.v[channelBase(1)+kChannelInsert1] = 1;
    f.v[routeParam(0,kRouteOutputL)] = 21;
    f.v[routeParam(0,kRouteReturnL)] = 7;
    f.v[kParamFx1L] = 17;
    f.v[channelBase(0)+kChannelSendAmount] = 25;
    f.offset(1,-100);
    f.offset(2,-200);
    const int base = millisecondsToSamples(20,NT_globals.sampleRate);
    for (int start=0; start<base+8; start+=4)
    {
        std::vector<float> buses(kNT_lastBus*4,0);
        if (start==0)
        {
            busSample(buses,1,0)=1;
            busSample(buses,2,0)=2;
            busSample(buses,3,0)=3;
            busSample(buses,7,0)=10;
        }
        step(f.alg,buses.data(),1);
        for (int n=0; n<4; ++n)
        {
            const int t=start+n;
            assertClose(busSample(buses,21,n),t==0?3:0);
            assertClose(busSample(buses,13,n),(t==0?10:0)+(t==base?3:0));
            assertClose(busSample(buses,17,n),t==0?5:0);
        }
    }
    assert(f.alg->sharedReturnDelays[0].requested == 0);
}

void testSharedReturnDuringInsertFade()
{
    OffsetFixture f(2);
    f.v[channelBase(0)+kChannelInputL] = 1;
    f.v[channelBase(1)+kChannelInputL] = 2;
    f.v[channelBase(0)+kChannelInsert1] = 1;
    f.v[routeParam(0,kRouteOutputL)] = 21;
    f.v[routeParam(0,kRouteReturnL)] = 7;
    std::vector<float> buses(kNT_lastBus*4,0);
    fillBus(buses,1,1); fillBus(buses,2,2); fillBus(buses,7,10);
    step(f.alg,buses.data(),1);
    assertBus(buses,13,12);
    f.v[kParamFadeMs] = 2;
    f.v[channelBase(1)+kChannelInsert1] = 1;
    for (int block=0; block<60; ++block)
    {
        buses.assign(kNT_lastBus*4,0);
        fillBus(buses,1,1); fillBus(buses,2,2); fillBus(buses,7,10);
        step(f.alg,buses.data(),1);
        for (int n=0; n<4; ++n)
        {
            const float main = busSample(buses,13,n);
            assert(main >= 9.999f && main <= 12.001f);
        }
    }
    assertBus(buses,13,10);
}

void testOffsetLiveChanges()
{
    OffsetFixture f(2);
    f.v[channelBase(0)+kChannelInputL]=1;
    int warm=millisecondsToSamples(40,NT_globals.sampleRate);
    for(int start=0;start<4*warm;start+=4)
    {
        if(start>=warm && f.alg->channelOffsets[1]==0) f.offset(2,-300);
        if(start>=2*warm && f.alg->channelOffsets[1]==-300) f.offset(2,-73);
        if(start>=3*warm && f.alg->channelOffsets[1]==-73) f.offset(2,0);
        std::vector<float> buses(kNT_lastBus*4,0); fillBus(buses,1,1);
        step(f.alg,buses.data(),1);
        assertBus(buses,13,1); // Warm history + crossfade must not drop/spike DC.
    }
}

int main()
{
    testOffsetEditorAndRestore(); testOffsetParameterLayouts(); testOffsetImpulseRouting();
    testOffsetSidechainAlignment(); testSharedFinalInsertReturn();
    testSharedReturnDuringInsertFade(); testOffsetLiveChanges();
    printf("PASS: offset editor, restore, 1..10-channel layouts, stereo impulses, inserts, FX, sidechain and live changes at %.0f Hz\n",double(NT_globals.sampleRate));
}
