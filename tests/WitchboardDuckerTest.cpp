#define main cleanTestMain
#include "WitchboardCleanTest.cpp"
#undef main

struct Fixture
{
 _NT_algorithmRequirements req = {};
 _NT_algorithmMemoryPtrs memory;
 WitchboardAlgorithm* alg;
 std::vector<int16_t> v;
 Fixture()
 {
  const int32_t specs[] = {2};
  calculateRequirements(req, specs);
  memory = allocateMemory(req);
  alg = static_cast<WitchboardAlgorithm*>(constructWitchboard(memory, req, specs));
  v.resize(req.numParameters);
  for (unsigned i = 0; i < req.numParameters; ++i) v[i] = alg->parameters[i].def;
  v[channelBase(0)+kChannelInputL] = 1; v[channelBase(0)+kChannelInputR] = 2;
  v[channelBase(1)+kChannelInputL] = 3; v[channelBase(1)+kChannelInputR] = 4;
  v[channelBase(1)+kChannelOutputPath] = kOutputPathBypass;
  v[kParamMainL] = 13; v[kParamMainR] = 14;
  v[kParamBypassL] = 15; v[kParamBypassR] = 16;
  v[kParamMasterSendL] = 17; v[kParamMasterSendR] = 18;
  v[kParamMasterReturnL] = 19; v[kParamMasterReturnR] = 20;
  v[kParamSidechainKeyInput] = 5;
  alg->v = v.data(); alg->vIncludingCommon = v.data();
 }
 ~Fixture() { freeMemory(memory); }
 void tick() { stepOnce(alg, v); }
};

void testEnvelope()
{
 const float rate = NT_globals.sampleRate;
 assertClose(envLengthFromNormalized(0),50);
 assertClose(envLengthFromNormalized(1),2000);
 assert(fabsf(envLengthFromNormalized(0.5f)-316.227766f)<0.001f);
 assert(fabsf(envLengthFromNormalized(0.486f)-300)<0.6f);
 for (int control : {0,250,486,750,1000})
 for (int curve = -100; curve <= 100; curve += 5)
 {
  const int n = millisecondsToSamples(envLengthFromNormalized(control*0.001f),rate);
  assert(n == static_cast<int>(round(envLengthFromNormalized(control*0.001f)*rate/1000)));
  const float beta = curveToBeta(curve*0.01f);
  SidechainRuntime sc = {};
  assertClose(processSidechain(sc,0,n,beta,0,1),1);
  assertClose(processSidechain(sc,5,n,beta,0,1),0);
  float last = 0;
  for (int i = 1; i <= n; ++i)
  {
   const float gain = processSidechain(sc,5,n,beta,0,1); // held key must not retrigger
   const double x = double(i)/n;
   const double expected = beta == 0 ? x : expm1(beta*x)/expm1(beta);
   assert(isfinite(gain) && gain >= last && gain <= 1);
   if (fabs(gain-expected) > 0.00001)
    fprintf(stderr,"curve %d n %d i %d: %.9f vs %.9f\n",curve,n,i,gain,expected);
   assert(fabs(gain-expected) < 0.00001);
   if (i == n/4 && curve < 0) assert(gain > x);
   if (i == n/4 && curve > 0) assert(gain < x);
   last = gain;
  }
  assert(sc.value == 1 && sc.remaining == 0 && sc.gain == 1);
  processSidechain(sc,0,n,beta,0,1);
  assertClose(processSidechain(sc,-5,n,beta,0,1),0);
 }
 for (float depth : {0.0f,0.5f,1.0f})
 {
  SidechainRuntime sc = {};
  assertClose(processSidechain(sc,5,100,0,0,depth),1-depth);
  for (int i=0;i<100;++i) processSidechain(sc,0,100,0,0,depth);
  assertClose(sc.gain,1);
 }
 SidechainRuntime sc = {};
 assertClose(processSidechain(sc,0.1f,100,0,0,1),1);
 assertClose(processSidechain(sc,0.101f,100,0,0,1),0);
 for(int i=0;i<50;++i) processSidechain(sc,0,100,0,0,1);
 assertClose(sc.gain,0.5f);
 assertClose(processSidechain(sc,5,100,0,0,1),0);
}

void testSmooth()
{
 const int n = millisecondsToSamples(4*2.0f, NT_globals.sampleRate);
 for (bool rise : {false,true})
 {
  RampSmoothRuntime ramp = {};
  ramp.value = ramp.target = rise ? 0.0f : 1.0f;
  for(int i=1;i<=n;++i)
  {
   float y = rampSmooth(ramp, rise ? 1 : 0, n);
   assert(fabsf(y-(rise ? float(i)/n : 1-float(i)/n)) < 0.00002f);
  }
  assert(ramp.value == (rise ? 1 : 0));
 }
 RampSmoothRuntime ramp = {};
 for(int i=0;i<1000;++i)
 {
  float input = 0.5f+0.5f*sinf(i*0.03f);
  assert(rampSmooth(ramp,input,0)==input);
 }
 SidechainRuntime sc = {};
 for (int i=0;i<100000;++i)
 {
  float y=processSidechain(sc,i%3001==0?5:0,5000,curveToBeta(-0.5f),n,0.8f);
  assert(isfinite(y) && y>=0.19999f && y<=1);
 }
}

void testDelay()
{
 const float rate=NT_globals.sampleRate;
 for (float ms : {0.0f,0.1f,6.0f,10.0f,100.0f})
 {
  std::vector<float> data(2*kBypassDelayCapacity);
  StereoDelay d = {}; d.data=data.data(); d.capacity=kBypassDelayCapacity;
  const int n=millisecondsToSamples(ms,rate);
  setDelay(d,n,millisecondsToSamples(5,rate));
  for(int i=0;i<n+2*kBypassDelayCapacity;++i)
  {
   float l=i==0?1:0, r=i==0?-0.25f:0;
   processDelay(d,l,r);
   assertClose(l,i==n?1:0); assertClose(r,i==n?-0.25f:0);
  }
 }
 // A hard tap jump on this 1 kHz signal would introduce a full-scale step.
 // Crossfade derivative is bounded by the input derivative plus 2/fadeSamples.
 std::vector<float> data(2*kBypassDelayCapacity);
 StereoDelay d = {}; d.data=data.data(); d.capacity=kBypassDelayCapacity;
 const int fade=millisecondsToSamples(5,rate);
 setDelay(d,0,fade);
 float previous=0;
 for(int i=0;i<int(rate);++i)
 {
  if(i>12000 && i%4==0) setDelay(d,(i*173)%kBypassDelayCapacity,fade);
  float l=sinf(2*kPi*1000*i/rate),r=-l;
  processDelay(d,l,r);
  assert(isfinite(l)); assertClose(l,-r);
  assert(fabsf(l-previous) <= 2*sinf(kPi*1000/rate)+2.0f/fade+0.001f);
  previous=l;
 }
 setDelay(d,0,fade);
 for(int i=0;i<2*fade+1;++i) {float l=0,r=0;processDelay(d,l,r);}
 assert(d.current==0 && d.fadePosition==0);
}

void testAutoFollow()
{
 Fixture f;
 f.tick();
 const int calls=setterCalls;
 f.v[kParamSidechainMode]=1;
 parameterChanged(f.alg,kParamSidechainMode);
 f.tick(); assert(f.v[kParamBypassOffset]==60);
 assert(setterCalls==calls+1);
 f.v[kParamBypassOffset]=100; f.tick();
 f.v[kParamSidechainLookahead]=80; f.tick(); // CV values consumed without callback too
 assert(f.v[kParamBypassOffset]==120);
 f.v[kParamBypassOffset]=130; parameterChanged(f.alg,kParamBypassOffset); f.tick();
 f.v[kParamSidechainLookahead]=60; parameterChanged(f.alg,kParamSidechainLookahead); f.tick();
 assert(f.v[kParamBypassOffset]==110);
 f.v[kParamSidechainMode]=0; f.tick(); assert(f.v[kParamBypassOffset]==50);
 f.v[kParamSidechainMode]=1; f.tick(); assert(f.v[kParamBypassOffset]==110);
 for(bool before : {false,true})
 {
  Fixture loaded;
  if(before) { _NT_jsonParse parse(nullptr,0); assert(deserialise(loaded.alg,parse)); }
  loaded.v=f.v;
  // Model the host restoring every parameter and notifying the plugin.
  for(unsigned i=0;i<loaded.v.size();++i) parameterChanged(loaded.alg,i);
  if(!before) { _NT_jsonParse parse(nullptr,0); assert(deserialise(loaded.alg,parse)); }
  loaded.tick(); assert(loaded.v[kParamBypassOffset]==110);
  loaded.v[kParamSidechainLookahead]=80; loaded.tick();
  assert(loaded.v[kParamBypassOffset]==130);
  // Also exercise deserialising into an instance which has already rendered.
  _NT_jsonParse parse(nullptr,0); assert(deserialise(loaded.alg,parse));
  loaded.v[kParamBypassOffset]=150; loaded.v[kParamSidechainLookahead]=40;
  loaded.tick(); assert(loaded.v[kParamBypassOffset]==150);
 }
 f.v[kParamBypassOffset]=1000; f.tick();
 f.v[kParamSidechainLookahead]=100; f.tick(); assert(f.v[kParamBypassOffset]==1000);
 f.v[kParamBypassOffset]=0; f.tick();
 f.v[kParamSidechainMode]=0; f.tick(); assert(f.v[kParamBypassOffset]==0);
 assert(f.alg->manualTrim==-100);
 // A negative trim survives SC OFF, preset save/load, then SC ON.
 copyText(f.alg->channelNames[0],kHardwareNameLength,"Kick");
 copyText(f.alg->routeNames[0],kHardwareNameLength,"iPad insert");
 copyText(f.alg->fxNames[1],kHardwareNameLength,"Stereo delay");
 copyText(f.alg->slotNames[1][2],kSlotNameLength,"Pedal");
 JsonTape tape;
 { _NT_jsonStream stream(&tape); stream.openObject(); serialise(f.alg,stream); stream.closeObject(); }
 Fixture restored;
 restored.v=f.v;
 { _NT_jsonParse parse(&tape,0); assert(deserialise(restored.alg,parse)); }
 restored.tick();
 assert(restored.alg->manualTrim==-100);
 restored.v[kParamSidechainMode]=1; restored.tick();
 assert(restored.v[kParamBypassOffset]==0);
 assert(strcmp(restored.alg->channelNames[0],"Kick")==0);
 assert(strcmp(restored.alg->parameterPages->pages[5].name,"Kick")==0);
 assert(strcmp(restored.alg->routeNames[0],"iPad insert")==0);
 assert(strcmp(restored.alg->fxNames[1],"Stereo delay")==0);
 assert(strcmp(restored.alg->slotNames[1][2],"Pedal")==0);
 // Upper clamp also preserves trim and is reversible.
 restored.v[kParamBypassOffset]=990; restored.tick();
 restored.v[kParamSidechainLookahead]=0; restored.tick();
 assert(restored.v[kParamBypassOffset]==890);
 restored.v[kParamBypassOffset]=1000; restored.tick();
 restored.v[kParamSidechainLookahead]=100; restored.tick();
 assert(restored.v[kParamBypassOffset]==1000);
 restored.v[kParamSidechainLookahead]=0; restored.tick();
 assert(restored.v[kParamBypassOffset]==1000);

}

void testRouting()
{
 const float rate=NT_globals.sampleRate;
 for(int mode=kMasterSplit;mode<=kMasterInsert;++mode)
 for(int branch=0;branch<2;++branch)
 for(bool fx : {false,true})
 for(bool enabled : {false,true})
 for(int lookahead : {0,60,100})
 {
  Fixture f;
  f.v[kParamMasterMode]=mode;
  f.v[kParamSidechainMode]=enabled;
  f.v[kParamSidechainDepth]=0;
  f.v[kParamSidechainLookahead]=lookahead;
  f.v[kParamBypassOffset]=enabled?lookahead:0;
  if(fx)
  {
   f.v[channelBase(0)+kChannelEnable]=0; f.v[channelBase(1)+kChannelEnable]=0;
   f.v[kParamFx1ReturnL]=1; f.v[kParamFx1ReturnR]=2;
   f.v[kParamFx1ReturnWidth]=kWidthStereo; f.v[kParamFx1ReturnPath]=branch;
  }
  const int n=enabled?millisecondsToSamples(lookahead*0.1f,rate):0;
  for(int block=0;block<(n+8)/4+1;++block)
  {
   std::vector<float> buses(kNT_lastBus*4,0);
   if(block==0)
   {
    busSample(buses,fx?1:1+2*branch,0)=1;
    busSample(buses,fx?2:2+2*branch,0)=-0.5f;
   }
   step(f.alg,buses.data(),1);
   const int out=mode==kMasterInsert?17:(mode==kMasterSplit&&branch?15:13);
   for(int i=0;i<4;++i)
   {
    const float expected=block*4+i==n?1:0;
    assertClose(busSample(buses,out,i),expected);
    assertClose(busSample(buses,out+1,i),-0.5f*expected);
    if(mode==kMasterInsert) assertClose(busSample(buses,13,i),0);
   }
  }
 }
 // Main receives undelayed triggers while audio waits in lookahead.
 Fixture f; f.v[kParamSidechainMode]=1; f.v[kParamSidechainSmooth]=0;
 f.v[kParamSidechainDepth]=100;
 std::vector<float> buses(kNT_lastBus*4,0);
 fillBus(buses,1,1); busSample(buses,5,0)=5;
 step(f.alg,buses.data(),1);
 assert(f.alg->sidechain.remaining==millisecondsToSamples(envLengthFromNormalized(0.486f),rate)-3);
 assertBus(buses,13,0);
 // Manual bypass delay still works with sidechain off, including 100 ms max.
 Fixture manual; manual.v[kParamBypassOffset]=1000;
 const int n=millisecondsToSamples(100,rate);
 for(int block=0;block<(n+8)/4;++block)
 {
  buses.assign(kNT_lastBus*4,0);
  if(block==0) {busSample(buses,1,0)=1;busSample(buses,3,0)=1;}
  step(manual.alg,buses.data(),1);
  for(int i=0;i<4;++i)
  {
   assertClose(busSample(buses,13,i),block*4+i==0?1:0);
   assertClose(busSample(buses,15,i),block*4+i==n?1:0);
  }
 }
}

void testFilterPlacement()
{
 // Isolate Bypass: only Sum/Insert should pass this branch through the filter.
 for(int mode=kMasterSplit;mode<=kMasterInsert;++mode)
 {
  Fixture f; f.v[kParamMasterMode]=mode;
  f.v[kParamMasterFilterEnable]=1; f.v[kParamMasterFilterSweep]=100;
  std::vector<float> buses(kNT_lastBus*4,0);
  fillBus(buses,3,1); fillBus(buses,4,-1);
  step(f.alg,buses.data(),1);
  if(mode==kMasterSplit) {assertBus(buses,15,1);assertBus(buses,16,-1);}
  else
  {
   const int out=mode==kMasterInsert?17:13;
   assert(fabsf(busSample(buses,out,0))<0.99f);
   assertClose(busSample(buses,out,0),-busSample(buses,out+1,0));
  }
 }
}

void testLiveIntegration()
{
 const float rate=NT_globals.sampleRate;
 const int fade=millisecondsToSamples(5,rate);
 Fixture f; f.v[kParamMasterMode]=kMasterSum; f.v[kParamSidechainDepth]=0;
 float previous=0;
 // Live SC toggles and both delay controls, with Main/Bypass populated while off.
 for(int block=0;block<4000;++block)
 {
  if(block==500) f.v[kParamSidechainMode]=1;
  if(block>600 && block<3000 && block%5==0)
  {
   f.v[kParamSidechainLookahead]=(block*3)%101;
   f.v[kParamBypassOffset]=(block*17)%1001;
  }
  if(block==3000) {f.v[kParamSidechainMode]=0;f.v[kParamBypassOffset]=0;}
  std::vector<float> buses(kNT_lastBus*4,0);
  for(int i=0;i<4;++i)
  {
   const float input=0.25f*sinf(2*kPi*1000*(block*4+i)/rate);
   busSample(buses,1,i)=input;busSample(buses,3,i)=input;
  }
  step(f.alg,buses.data(),1);
  for(int i=0;i<4;++i)
  {
   const float y=busSample(buses,13,i);
   assert(isfinite(y));
   assert(fabsf(y-previous)<=sinf(kPi*1000/rate)+1.0f/fade+0.001f);
   if(block>=3500) assertClose(y,2*busSample(buses,1,i));
   previous=y;
  }
 }
 assert(f.alg->mainDelay.current==0 && f.alg->bypassDelay.current==0);
}

void testBlockSize()
{
 Fixture small, large;
 for(auto* f : {&small,&large})
 {
  f->v[kParamSidechainMode]=1;
  f->v[kParamBypassOffset]=60;
  f->v[kParamMasterMode]=kMasterSum;
 }
 for(int start=0;start<8192;start+=64)
 {
  std::vector<float> b(kNT_lastBus*64,0),expected(64);
  for(int i=0;i<64;++i)
  {
   b[i]=0.4f*sinf((start+i)*0.09f);
   b[2*64+i]=0.2f*cosf((start+i)*0.03f);
   b[4*64+i]=(start+i)%401==0?5:0;
  }
  for(int offset=0;offset<64;offset+=4)
  {
   std::vector<float> a(kNT_lastBus*4,0);
   for(int bus=0;bus<kNT_lastBus;++bus)
    for(int i=0;i<4;++i) a[bus*4+i]=b[bus*64+offset+i];
   step(small.alg,a.data(),1);
   for(int i=0;i<4;++i) expected[offset+i]=busSample(a,13,i);
  }
  step(large.alg,b.data(),16);
  for(int i=0;i<64;++i) assertClose(b[12*64+i],expected[i]);
 }
 char label[kNT_parameterStringSize];
 parameterString(large.alg,kParamSidechainEnvLength,0,label);
 assert(strcmp(label,"50 ms")==0);
 parameterString(large.alg,kParamSidechainEnvLength,1000,label);
 assert(strcmp(label,"2000 ms")==0);
}

int main()
{
 testEnvelope(); testSmooth(); testDelay(); testAutoFollow(); testRouting();
 testFilterPlacement(); testLiveIntegration(); testBlockSize();
 printf("PASS: v1.34 envelope, smooth, depth, stereo delays, live taps, auto-follow, load callbacks, FX and Split/Sum/Insert at %.0f Hz\n",double(NT_globals.sampleRate));
}
