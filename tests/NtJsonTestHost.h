// Token-level host adapter for exercising the plugin's actual serialise/deserialise
// callbacks, including nested route/FX/slot names and optional latency metadata.
#include <string>

struct JsonToken
{
 char kind;
 std::string text;
 int number, count, next;
};
struct JsonTape
{
 std::vector<JsonToken> tokens;
 std::vector<int> stack;
 void add(char kind, const char* text = "", int number = 0)
 {
  if (!stack.empty())
  {
   auto& parent = tokens[stack.back()];
   if (parent.kind == 'a' || (parent.kind == 'o' && kind == 'k')) ++parent.count;
  }
  const int index = tokens.size();
  tokens.push_back({kind, text, number, 0, index+1});
  if (kind == 'a' || kind == 'o') stack.push_back(index);
 }
 void close()
 {
  assert(!stack.empty());
  tokens[stack.back()].next = tokens.size();
  stack.pop_back();
 }
};
_NT_jsonStream::_NT_jsonStream(void* r) : refCon(r) {}
_NT_jsonStream::~_NT_jsonStream() {}
void _NT_jsonStream::openArray() { static_cast<JsonTape*>(refCon)->add('a'); }
void _NT_jsonStream::closeArray() { static_cast<JsonTape*>(refCon)->close(); }
void _NT_jsonStream::openObject() { static_cast<JsonTape*>(refCon)->add('o'); }
void _NT_jsonStream::closeObject() { static_cast<JsonTape*>(refCon)->close(); }
void _NT_jsonStream::addMemberName(const char* s) { static_cast<JsonTape*>(refCon)->add('k',s); }
void _NT_jsonStream::addNumber(int n) { static_cast<JsonTape*>(refCon)->add('n',"",n); }
void _NT_jsonStream::addNumber(float) { assert(false); }
void _NT_jsonStream::addString(const char* s) { static_cast<JsonTape*>(refCon)->add('s',s); }
void _NT_jsonStream::addFourCC(uint32_t) { assert(false); }
void _NT_jsonStream::addBoolean(bool) { assert(false); }
void _NT_jsonStream::addNull() { static_cast<JsonTape*>(refCon)->add('z'); }

_NT_jsonParse::_NT_jsonParse(void* r, int pos) : refCon(r), i(pos) {}
_NT_jsonParse::~_NT_jsonParse() {}
static const JsonToken* peekToken(void* r, int i)
{
 if (!r) return nullptr;
 auto& tape = *static_cast<JsonTape*>(r);
 return i>=0 && unsigned(i)<tape.tokens.size() ? &tape.tokens[i] : nullptr;
}
bool _NT_jsonParse::numberOfArrayElements(int& n)
{
 const auto* t = peekToken(refCon,i);
 if (!t || t->kind!='a') return false;
 n=t->count; ++i; return true;
}
bool _NT_jsonParse::numberOfObjectMembers(int& n)
{
 if (!refCon) { n=0; return true; } // empty custom preset object
 const auto* t = peekToken(refCon,i);
 if (!t || t->kind!='o') return false;
 n=t->count; ++i; return true;
}
bool _NT_jsonParse::matchName(const char* s)
{
 const auto* t = peekToken(refCon,i);
 if (!t || t->kind!='k' || t->text!=s) return false;
 ++i; return true;
}
bool _NT_jsonParse::skipMember()
{
 const auto* t = peekToken(refCon,i);
 if (!t || t->kind!='k') return false;
 t=peekToken(refCon,++i);
 if (!t) return false;
 i=t->next; return true;
}
bool _NT_jsonParse::number(int& n)
{
 const auto* t = peekToken(refCon,i);
 if (!t || t->kind!='n') return false;
 n=t->number; ++i; return true;
}
bool _NT_jsonParse::number(float&) { return false; }
bool _NT_jsonParse::string(const char*& s)
{
 const auto* t = peekToken(refCon,i);
 if (!t || t->kind!='s') return false;
 s=t->text.c_str(); ++i; return true;
}
bool _NT_jsonParse::boolean(bool&) { return false; }
bool _NT_jsonParse::null() { return false; }
