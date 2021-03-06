// test.cpp - written and placed in the public domain by Wei Dai

#define CRYPTOPP_DEFAULT_NO_DLL
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "dll.h"
#include "aes.h"
#include "cryptlib.h"
#include "filters.h"
#include "md5.h"
#include "ripemd.h"
#include "rng.h"
#include "gzip.h"
#include "default.h"
#include "randpool.h"
#include "ida.h"
#include "base64.h"
#include "socketft.h"
#include "wait.h"
#include "factory.h"
#include "whrlpool.h"
#include "tiger.h"
#include "smartptr.h"
#include "ossig.h"
#include "trap.h"

#include "validate.h"
#include "bench.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <locale>
#include <time.h>

//naim's code
#include <cstdio>


char *_plainTextFilePath = "C:\\Users\\nagabe\\Desktop\\Xenakis\\new 1.txt";
char *_cipherTextFilePath = "C:\\Users\\nagabe\\Desktop\\Xenakis\\new 2.txt";
char *_publicKeyFilePath = "C:\\Users\\nagabe\\Desktop\\Xenakis\\publicKey.txt";
char *_privateKeyFilePath = "C:\\Users\\nagabe\\Desktop\\Xenakis\\privateKey.txt";
char *_message = "message";
char *_passPhrase = "password";
char *_charSeed = "seed";

std::string _stringSeed = "seed";

int _keySize = 1024;



#ifdef CRYPTOPP_WIN32_AVAILABLE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(USE_BERKELEY_STYLE_SOCKETS) && !defined(macintosh)
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#if (_MSC_VER >= 1000)
#include <crtdbg.h>		// for the debug heap
#endif

#if defined(__MWERKS__) && defined(macintosh)
#include <console.h>
#endif

#ifdef _OPENMP
# include <omp.h>
#endif

#ifdef __BORLANDC__
#pragma comment(lib, "cryptlib_bds.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

// Aggressive stack checking with VS2005 SP1 and above.
#if (CRYPTOPP_MSC_VERSION >= 1410)
# pragma strict_gs_check (on)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

const int MAX_PHRASE_LENGTH = 250;

void RegisterFactories();
void PrintSeedAndThreads(const std::string& seed);

void GenerateRSAKey(unsigned int keyLength, const char *privFilename, const char *pubFilename, const char *seed);
string RSAEncryptString(const char *pubFilename, const char *seed, const char *message);
string RSADecryptString(const char *privFilename, const char *ciphertext);

void DigestFile(const char *file);
void HmacFile(const char *hexKey, const char *file);

void AES_CTR_Encrypt(const char *hexKey, const char *hexIV, const char *infile, const char *outfile);

string EncryptString(const char *plaintext, const char *passPhrase);
string DecryptString(const char *ciphertext, const char *passPhrase);

void Base64Encode(const char *infile, const char *outfile);
void Base64Decode(const char *infile, const char *outfile);
void HexEncode(const char *infile, const char *outfile);
void HexDecode(const char *infile, const char *outfile);

void FIPS140_SampleApplication();
void FIPS140_GenerateRandomFiles();

void EncryptDecryptMAC();

void EncryptDecryptWithRsa();

bool Validate(int, bool, const char *);
void PrintSeedAndThreads(const std::string& seed);

int(*AdhocTest)(int argc, char *argv[]) = NULL;

namespace { OFB_Mode<AES>::Encryption s_globalRNG; }
RandomNumberGenerator & GlobalRNG()
{
	return dynamic_cast<RandomNumberGenerator&>(s_globalRNG);
}

// See misc.h and trap.h for comments and usage
#if CRYPTOPP_DEBUG && (defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE))
static const SignalHandler<SIGTRAP, false> s_dummyHandler;
// static const DebugTrapHandler s_dummyHandler;
#endif

int main(int argc, char *argv[])
{
	try
	{
		RegisterFactories();

		// Some editors have problems with the '\0' character when redirecting output.
		std::string seed = IntToString(time(NULL));
		seed.resize(16, ' ');

		OFB_Mode<AES>::Encryption& prng = dynamic_cast<OFB_Mode<AES>::Encryption&>(GlobalRNG());
		prng.SetKeyWithIV((byte *)seed.data(), 16, (byte *)seed.data());

		GenerateRSAKey(_keySize, _privateKeyFilePath, _publicKeyFilePath, _charSeed);
		EncryptDecryptWithRsa();

		DigestFile(argv[2]);
		EncryptDecryptMAC();

		Base64Encode(_plainTextFilePath, _cipherTextFilePath);
		Base64Decode(_cipherTextFilePath, _plainTextFilePath);

		HexEncode(_plainTextFilePath, _cipherTextFilePath);
		HexDecode(_cipherTextFilePath, _plainTextFilePath);

		std::getchar();
		return 0;
	}
	catch (const CryptoPP::Exception &e)
	{
		cout << "\nCryptoPP::Exception caught: " << e.what() << endl;
		return -1;
	}
	catch (const std::exception &e)
	{
		cout << "\nstd::exception caught: " << e.what() << endl;
		return -2;
	}
} // End main()

void EncryptDecryptWithRsa()
{
	cout << "\Plaintext: " << _message << endl;
	string ciphertext = RSAEncryptString(_publicKeyFilePath, _charSeed, _message);
	cout << "\nCiphertext: " << ciphertext << endl;

	string decrypted = RSADecryptString(_privateKeyFilePath, ciphertext.c_str());
	cout << "\nDecrypted: " << decrypted << endl;
}

void EncryptDecryptMAC()
{

	string ciphertext = EncryptString(_message, _passPhrase);
	cout << "\nCiphertext: " << ciphertext << endl;
	cout << "\Passphrase: " << _passPhrase << endl;

	string decrypted = DecryptString(ciphertext.c_str(), _passPhrase);
	cout << "\nDecrypted: " << decrypted << endl;
}

void FIPS140_GenerateRandomFiles()
{
#ifdef OS_RNG_AVAILABLE
	DefaultAutoSeededRNG rng;
	RandomNumberStore store(rng, ULONG_MAX);

	for (unsigned int i = 0; i < 100000; i++)
		store.TransferTo(FileSink((IntToString(i) + ".rnd").c_str()).Ref(), 20000);
#else
	cout << "OS provided RNG not available.\n";
	exit(-1);
#endif
}

template <class T, bool NON_NEGATIVE>
T StringToValue(const std::string& str) {
	std::istringstream iss(str);

	// Arbitrary, but we need to clear a Coverity finding TAINTED_SCALAR
	if (iss.str().length() > 25)
		throw InvalidArgument("cryptest.exe: '" + str + "' is tool ong");

	T value;
	iss >> std::noskipws >> value;

	// Use fail(), not bad()
	if (iss.fail() || !iss.eof())
		throw InvalidArgument("cryptest.exe: '" + str + "' is not a value");

	if (NON_NEGATIVE && value < 0)
		throw InvalidArgument("cryptest.exe: '" + str + "' is negative");

	return value;
}

template<>
int StringToValue<int, true>(const std::string& str)
{
	Integer n(str.c_str());
	long l = n.ConvertToLong();

	int r;
	if (!SafeConvert(l, r))
		throw InvalidArgument("cryptest.exe: '" + str + "' is not an integer value");

	return r;
}

void PrintSeedAndThreads(const std::string& seed)
{
	cout << "Using seed: " << seed << endl;

#ifdef _OPENMP
	int tc = 0;
#pragma omp parallel
	{
		tc = omp_get_num_threads();
	}

	std::cout << "Using " << tc << " OMP " << (tc == 1 ? "thread" : "threads") << std::endl;
#endif
}

SecByteBlock HexDecodeString(const char *hex)
{
	StringSource ss(hex, true, new HexDecoder);
	SecByteBlock result((size_t)ss.MaxRetrievable());
	ss.Get(result, result.size());
	return result;
}

void GenerateRSAKey(unsigned int keyLength, const char *privFilename, const char *pubFilename, const char *seed)
{
	RandomPool randPool;
	randPool.IncorporateEntropy((byte *)seed, strlen(seed));

	RSAES_OAEP_SHA_Decryptor priv(randPool, keyLength);
	HexEncoder privFile(new FileSink(privFilename));
	priv.DEREncode(privFile);
	privFile.MessageEnd();

	RSAES_OAEP_SHA_Encryptor pub(priv);
	HexEncoder pubFile(new FileSink(pubFilename));
	pub.DEREncode(pubFile);
	pubFile.MessageEnd();
}

string RSAEncryptString(const char *pubFilename, const char *seed, const char *message)
{
	FileSource pubFile(pubFilename, true, new HexDecoder);
	RSAES_OAEP_SHA_Encryptor pub(pubFile);

	RandomPool randPool;
	randPool.IncorporateEntropy((byte *)seed, strlen(seed));

	string result;
	StringSource(message, true, new PK_EncryptorFilter(randPool, pub, new HexEncoder(new StringSink(result))));
	return result;
}

string RSADecryptString(const char *privFilename, const char *ciphertext)
{
	FileSource privFile(privFilename, true, new HexDecoder);
	RSAES_OAEP_SHA_Decryptor priv(privFile);

	string result;
	StringSource(ciphertext, true, new HexDecoder(new PK_DecryptorFilter(GlobalRNG(), priv, new StringSink(result))));
	return result;
}

void DigestFile(const char *filename)
{
	SHA1 sha;
	RIPEMD160 ripemd;
	SHA256 sha256;
	Tiger tiger;
	SHA512 sha512;
	Whirlpool whirlpool;
	vector_member_ptrs<HashFilter> filters(6);
	filters[0].reset(new HashFilter(sha));
	filters[1].reset(new HashFilter(ripemd));
	filters[2].reset(new HashFilter(tiger));
	filters[3].reset(new HashFilter(sha256));
	filters[4].reset(new HashFilter(sha512));
	filters[5].reset(new HashFilter(whirlpool));

	member_ptr<ChannelSwitch> channelSwitch(new ChannelSwitch);
	size_t i;
	for (i = 0; i < filters.size(); i++)
		channelSwitch->AddDefaultRoute(*filters[i]);
	FileSource(filename, true, channelSwitch.release());

	HexEncoder encoder(new FileSink(cout), false);
	for (i = 0; i < filters.size(); i++)
	{
		cout << filters[i]->AlgorithmName() << ": ";
		filters[i]->TransferTo(encoder);
		cout << "\n";
	}
}

void HmacFile(const char *hexKey, const char *file)
{
	member_ptr<MessageAuthenticationCode> mac;
	if (strcmp(hexKey, "selftest") == 0)
	{
		cerr << "Computing HMAC/SHA1 value for self test.\n";
		mac.reset(NewIntegrityCheckingMAC());
	}
	else
	{
		std::string decodedKey;
		StringSource(hexKey, true, new HexDecoder(new StringSink(decodedKey)));
		mac.reset(new HMAC<SHA1>((const byte *)decodedKey.data(), decodedKey.size()));
	}
	FileSource(file, true, new HashFilter(*mac, new HexEncoder(new FileSink(cout))));
}

void AES_CTR_Encrypt(const char *hexKey, const char *hexIV, const char *infile, const char *outfile)
{
	SecByteBlock key = HexDecodeString(hexKey);
	SecByteBlock iv = HexDecodeString(hexIV);
	CTR_Mode<AES>::Encryption aes(key, key.size(), iv);
	FileSource(infile, true, new StreamTransformationFilter(aes, new FileSink(outfile)));
}

string EncryptString(const char *instr, const char *passPhrase)
{
	string outstr;

	DefaultEncryptorWithMAC encryptor(passPhrase, new HexEncoder(new StringSink(outstr)));
	encryptor.Put((byte *)instr, strlen(instr));
	encryptor.MessageEnd();

	return outstr;
}

string DecryptString(const char *instr, const char *passPhrase)
{
	string outstr;

	HexDecoder decryptor(new DefaultDecryptorWithMAC(passPhrase, new StringSink(outstr)));
	decryptor.Put((byte *)instr, strlen(instr));
	decryptor.MessageEnd();

	return outstr;
}

void Base64Encode(const char *in, const char *out)
{
	FileSource(in, true, new Base64Encoder(new FileSink(out)));
}

void Base64Decode(const char *in, const char *out)
{
	FileSource(in, true, new Base64Decoder(new FileSink(out)));
}

void HexEncode(const char *in, const char *out)
{
	FileSource(in, true, new HexEncoder(new FileSink(out)));
}

void HexDecode(const char *in, const char *out)
{
	FileSource(in, true, new HexDecoder(new FileSink(out)));
}

bool Validate(int alg, bool thorough, const char *seedInput)
{
	bool result;

	// Some editors have problems with the '\0' character when redirecting output.
	//   seedInput is argv[3] when issuing 'cryptest.exe v all <seed>'
	std::string seed = (seedInput ? seedInput : IntToString(time(NULL)));
	seed.resize(16, ' ');

	OFB_Mode<AES>::Encryption& prng = dynamic_cast<OFB_Mode<AES>::Encryption&>(GlobalRNG());
	prng.SetKeyWithIV((byte *)seed.data(), 16, (byte *)seed.data());

	PrintSeedAndThreads(seed);

	switch (alg)
	{
	case 0: result = ValidateAll(thorough); break;
	case 1: result = TestSettings(); break;
	case 2: result = TestOS_RNG(); break;
	case 3: result = ValidateMD5(); break;
	case 4: result = ValidateSHA(); break;
	case 5: result = ValidateDES(); break;
	case 6: result = ValidateIDEA(); break;
	case 7: result = ValidateARC4(); break;
	case 8: result = ValidateRC5(); break;
	case 9: result = ValidateBlowfish(); break;
		//	case 10: result = ValidateDiamond2(); break;
	case 11: result = ValidateThreeWay(); break;
	case 12: result = ValidateBBS(); break;
	case 13: result = ValidateDH(); break;
	case 14: result = ValidateRSA(); break;
	case 15: result = ValidateElGamal(); break;
	case 16: result = ValidateDSA(thorough); break;
		//	case 17: result = ValidateHAVAL(); break;
	case 18: result = ValidateSAFER(); break;
	case 19: result = ValidateLUC(); break;
	case 20: result = ValidateRabin(); break;
		//	case 21: result = ValidateBlumGoldwasser(); break;
	case 22: result = ValidateECP(); break;
	case 23: result = ValidateEC2N(); break;
		//	case 24: result = ValidateMD5MAC(); break;
	case 25: result = ValidateGOST(); break;
	case 26: result = ValidateTiger(); break;
	case 27: result = ValidateRIPEMD(); break;
	case 28: result = ValidateHMAC(); break;
		//	case 29: result = ValidateXMACC(); break;
	case 30: result = ValidateSHARK(); break;
	case 32: result = ValidateLUC_DH(); break;
	case 33: result = ValidateLUC_DL(); break;
	case 34: result = ValidateSEAL(); break;
	case 35: result = ValidateCAST(); break;
	case 36: result = ValidateSquare(); break;
	case 37: result = ValidateRC2(); break;
	case 38: result = ValidateRC6(); break;
	case 39: result = ValidateMARS(); break;
	case 40: result = ValidateRW(); break;
	case 41: result = ValidateMD2(); break;
	case 42: result = ValidateNR(); break;
	case 43: result = ValidateMQV(); break;
	case 44: result = ValidateRijndael(); break;
	case 45: result = ValidateTwofish(); break;
	case 46: result = ValidateSerpent(); break;
	case 47: result = ValidateCipherModes(); break;
	case 48: result = ValidateCRC32(); break;
	case 49: result = ValidateECDSA(); break;
	case 50: result = ValidateXTR_DH(); break;
	case 51: result = ValidateSKIPJACK(); break;
	case 52: result = ValidateSHA2(); break;
	case 53: result = ValidatePanama(); break;
	case 54: result = ValidateAdler32(); break;
	case 55: result = ValidateMD4(); break;
	case 56: result = ValidatePBKDF(); break;
	case 57: result = ValidateESIGN(); break;
	case 58: result = ValidateDLIES(); break;
	case 59: result = ValidateBaseCode(); break;
	case 60: result = ValidateSHACAL2(); break;
	case 61: result = ValidateCamellia(); break;
	case 62: result = ValidateWhirlpool(); break;
	case 63: result = ValidateTTMAC(); break;
	case 64: result = ValidateSalsa(); break;
	case 65: result = ValidateSosemanuk(); break;
	case 66: result = ValidateVMAC(); break;
	case 67: result = ValidateCCM(); break;
	case 68: result = ValidateGCM(); break;
	case 69: result = ValidateCMAC(); break;
	case 70: result = ValidateHKDF(); break;
	case 71: result = ValidateBLAKE2s(); break;
	case 72: result = ValidateBLAKE2b(); break;
	default: return false;
	}

	// Safer functions on Windows for C&A, https://github.com/weidai11/cryptopp/issues/55
#if (CRYPTOPP_MSC_VERSION >= 1400)
	tm localTime = {};
	char timeBuf[64];
	errno_t err;

	const time_t endTime = time(NULL);
	err = localtime_s(&localTime, &endTime);
	CRYPTOPP_ASSERT(err == 0);
	err = asctime_s(timeBuf, sizeof(timeBuf), &localTime);
	CRYPTOPP_ASSERT(err == 0);

	cout << "\nTest ended at " << timeBuf;
#else
	const time_t endTime = time(NULL);
	cout << "\nTest ended at " << asctime(localtime(&endTime));
#endif

	cout << "Seed used was: " << seed << endl;

	return result;
}
