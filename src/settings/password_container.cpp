#include "settings/password_container.h"
#include "configuration/cryptography/certificate_store.h"
#include "configuration/cryptography/password_encryption.h"
#include "configuration/cryptography/public_key_encryption.h"
#include "configuration/cryptography/public_key_symmetric_encryption.h"

#include "seb_settings.h" // for typedef PasswordProvider

#include <QByteArray>
#include <QCryptographicHash>
#include <QDataStream>
#include <QMessageAuthenticationCode>
#include <QSslCertificate>
#include <QVariant>
#include <cstring>
#include <openssl/evp.h>
#include <vector>
#include <zlib.h>

using namespace seb::configuration::cryptography;

namespace seb::settingsinternal {
namespace {

QString sha256Hex(const QByteArray &data) {
  return QString::fromLatin1(
      QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

QString hashPassword(const QString &password) {
  return sha256Hex(password.toUtf8());
}

QByteArray inflateGzip(const QByteArray &data, QString *error) {
  z_stream stream{};
  stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
  stream.avail_in = static_cast<uInt>(data.size());

  if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
    if (error) {
      *error = QStringLiteral("Failed to initialize gzip decompression.");
    }
    return {};
  }

  QByteArray output;
  char buffer[8192];
  int status = Z_OK;

  while (status == Z_OK) {
    stream.next_out = reinterpret_cast<Bytef *>(buffer);
    stream.avail_out = sizeof(buffer);
    status = inflate(&stream, Z_NO_FLUSH);

    if (status != Z_OK && status != Z_STREAM_END) {
      if (error) {
        *error =
            QStringLiteral("Failed to decompress gzip-compressed SEB data.");
      }
      inflateEnd(&stream);
      return {};
    }

    output.append(buffer, static_cast<int>(sizeof(buffer) - stream.avail_out));
  }

  inflateEnd(&stream);
  return output;
}

constexpr int kSebPrefixLength          = 4;
constexpr int kSebCustomHeaderLength    = 4;
constexpr int kSebMultipartLength       = 8;
constexpr int kSebRncryptorHeaderLength = 2;
constexpr int kSebRncryptorSaltLength   = 8;
constexpr int kSebRncryptorIvLength     = 16;
constexpr int kSebRncryptorKeyLength    = 32;
constexpr int kSebRncryptorIterations   = 10000;
constexpr int kSebRncryptorVersion2     = 0x2;
constexpr int kSebRncryptorVersion3     = 0x3;
constexpr int kSebRncryptorOptions      = 0x1;

const QByteArray kPrefixPublicKey               = "pkhs";
const QByteArray kPrefixPublicKeySymmetric      = "phsk";
const QByteArray kPrefixPassword                = "pswd";
const QByteArray kPrefixPasswordConfigureClient = "pwcc";
const QByteArray kPrefixPlain                   = "plnd";
const QByteArray kPrefixMultipart               = "mphd";
const QByteArray kPrefixCustomHeader            = "cmhd";



/*
 * Return a vector with one or two elements.
 *
 * - Always:     password.toUtf8()
 * - Optionally: a truncated verision of password.toUtf8()
 *  
 */
std::vector<QByteArray> rncryptorPasswordByteCandidates(
    const QString& password
  , unsigned char  version
)
{
  const QByteArray        passwordBytes = password.toUtf8();
  std::vector<QByteArray> candidates;

  if ( version == kSebRncryptorVersion2
       && passwordBytes.size() > password.size()
      )
    {
      // RNCryptor v2 accidentally used the character count as the byte count
      // when deriving keys. For ASCII passwords this is identical to v3.
      candidates.push_back( passwordBytes.left( password.size() ) );
    }

  candidates.push_back( passwordBytes );
  return candidates;
}

QByteArray deriveKey( const QByteArray& passwordBytes
                    , const QByteArray& salt
                    )
{
  QByteArray key( kSebRncryptorKeyLength, Qt::Uninitialized );
  // /usr/include/openssl/evp.h : PKCS5_PBKDF2_HMAC_SHA1
  PKCS5_PBKDF2_HMAC_SHA1(
      passwordBytes.constData()  // pass
    , passwordBytes.size()       // passlen
    , reinterpret_cast<const unsigned char *>( salt.constData() ) // salt
    , salt.size()                                                 // saltlen
    , kSebRncryptorIterations    // iter
    , kSebRncryptorKeyLength     // keylen
    , reinterpret_cast<unsigned char *>( key.data() ) // OUT
  );
  return key;
}

bool isSupportedRncryptorHeader( unsigned char version
                               , unsigned char options )
{
  return (  version == kSebRncryptorVersion2
         || version == kSebRncryptorVersion3
         ) && options == kSebRncryptorOptions
         ;
}

/*
 *
 * @return false on failure
 */
bool decryptPasswordBlock(
   const QByteArray& data       // data to be decrypted
 , const QString&    password
 ,       QByteArray* decrypted  // out
 ,       QString*    error      // out
 )
{
  // Check if headers + trailer fit in data.
  const int headerLength = (
            kSebRncryptorHeaderLength
      + 2 * kSebRncryptorSaltLength
      +     kSebRncryptorIvLength
  );
  const int hmacLength = 32;
  const int encryptedPayloadLength = data.size() - headerLength - hmacLength ;
  
  if ( encryptedPayloadLength < 0 ) {
      if (error) {
        *error = QStringLiteral("The encrypted SEB payload is truncated.");
      }
    return false;
  }

  // Check the first kSebRncryptorHeaderLength=2 bytes
  const unsigned char version = static_cast<unsigned char>( data.at(0) );
  const unsigned char options = static_cast<unsigned char>( data.at(1) );
  if ( !isSupportedRncryptorHeader(version, options) ) {
    if (error) {
      *error = QStringLiteral("Unsupported encrypted SEB format version.");
    }
    return false;
  }

  // offset : offset of the next bytes in data
  int offset = kSebRncryptorHeaderLength;

#define TAKE( len ) data.mid(offset, len);   offset += len;
  //
  // const QByteArray encryptionSalt = data.mid(offset, kSebRncryptorSaltLength);  offset += kSebRncryptorSaltLength;
  // const QByteArray authenticationSalt = data.mid(offset, kSebRncryptorSaltLength); offset += kSebRncryptorSaltLength;
  // const QByteArray iv = data.mid(offset, kSebRncryptorIvLength);  offset += kSebRncryptorIvLength;
  // const QByteArray encryptedPayload = data.mid( offset, encryptedPayloadLength );
  // const QByteArray originalHmac = data.right(hmacLength); // The last len bytes of `data`

  const QByteArray encryptionSalt     = TAKE( kSebRncryptorSaltLength );
  const QByteArray authenticationSalt = TAKE( kSebRncryptorSaltLength );
  const QByteArray iv                 = TAKE( kSebRncryptorIvLength   );
  const QByteArray encryptedPayload   = TAKE( encryptedPayloadLength  );
  const QByteArray originalHmac       = TAKE( hmacLength              ); // The last len bytes of `data`
  // assert( offset == data.size() );
#undef TAKE  

  // Check originalHmac. On success, calculate encryptionKey 
  QByteArray encryptionKey;
  {
    // scope authenticated
    bool authenticated = false; // originalHmac passed

    // passwordByteCandidates : Due to an earlier mishap, we may need
    //                          to check multiple possibe passwordBytes.
    std::vector<QByteArray> passwordByteCandidates =
      rncryptorPasswordByteCandidates( password, version );
    //
    for ( const QByteArray& passwordBytes : passwordByteCandidates )
      {
        const QByteArray authenticationKey = deriveKey( passwordBytes
                                                      , authenticationSalt
                                                      );
        const QByteArray computedHmac = QMessageAuthenticationCode::hash(
              data.left( data.size() - hmacLength ) // message
            , authenticationKey                     // key
            , QCryptographicHash::Sha256            // method
         );
    
        if ( computedHmac == originalHmac ) {
          authenticated = true;
          //
          // Found the passwordBytes used.
          // Use this one for the encryptionKey
          //
          encryptionKey = deriveKey( passwordBytes
                                   , encryptionSalt
                                   );
          break;
        }
      }
  
    if (! authenticated ) {
      if (error) {
        *error =
            QStringLiteral("Invalid password or corrupted encrypted SEB data.");
      }
      return false;
    }
  }

  // EVP_CIPHER_CTX_new() Allocate a cipher context.
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if ( !ctx ) {
    if ( error ) {
      *error = QStringLiteral( "Failed to decrypt the SEB payload." );
      // ^^^ An obscure message for allocation failure.
    }
    return false;
  }

  // clear as in "decrypted"
  QByteArray clear( encryptedPayload.size() + kSebRncryptorIvLength
                  , Qt::Uninitialized
                  );

  // https://stackoverflow.com/questions/39412760/what-is-an-openssl-iv-and-why-do-i-need-a-key-and-an-iv#39412887
  //
  // Note: iv : Initialization Vector.
  //            IVs are part of what makes AES in CBC (Cipher Block Chaining) mode work.
  //            IVs should be random.
  //            IVs should not be reused.
  //            Every record should have its own IV.
  //            The IV is not a secret like the key. It can be
  //            stored in plaintext along with the cipher text.
  //
  int firstChunkLength = 0;
  int finalChunkLength = 0;
  const bool ok =
      EVP_DecryptInit_ex(
            ctx                     // cypher context
          , EVP_aes_256_cbc()       // cypher
          , nullptr                 // engine
          , reinterpret_cast<const unsigned char *>(encryptionKey.constData()) // key
          , reinterpret_cast<const unsigned char *>(iv.constData())            // iv
      ) == 1
    &&
      EVP_DecryptUpdate(
            ctx
          , reinterpret_cast<unsigned char *>(clear.data())                       // out
          , &firstChunkLength                                                     // outl
          , reinterpret_cast<const unsigned char *>(encryptedPayload.constData()) // in
          , encryptedPayload.size()                                               // inl
      ) == 1
    &&
      EVP_DecryptFinal_ex(
            ctx
          , reinterpret_cast<unsigned char *>(clear.data()) + firstChunkLength // outm
          , &finalChunkLength                                                  // outl
      ) == 1
    ;
  EVP_CIPHER_CTX_free(ctx);

  if (!ok) {
    if (error) {
      *error = QStringLiteral("Failed to decrypt the SEB payload.");
    }
    return false;
  }

  clear.truncate( firstChunkLength + finalChunkLength );
  if ( decrypted ) {
    *decrypted = clear;
  }
  return true;
}

} // namespace

bool looksLikeHtml( const QByteArray &raw ) {
  const QByteArray trimmed = raw.trimmed().left(256).toLower();
  return (  trimmed.startsWith("<!doctype html")
         || trimmed.startsWith("<html")
         );
}

bool startsWithXml( const QByteArray &data ) {
  const QByteArray trimmed = data.trimmed();
  return (  trimmed.startsWith("<?xml")
         || trimmed.startsWith("<plist")
         );
}

QByteArray readPrefix(const QByteArray& data) {
  return data.left( kSebPrefixLength );
}

/*
 *  @return Empty QByteArray on error.
 */
QByteArray
unwrapSebContainer( QByteArray   raw
                  , QString*     error       // out
                  , QStringList* warnings    // out
                  , seb::PasswordProvider passwordProvider
                  )
{
  while ( raw.size() >= kSebPrefixLength ) {
    
    const QByteArray prefix = readPrefix(raw);

    if ( prefix == kPrefixMultipart /* "mphd" */ ) {

      if ( raw.size() < kSebPrefixLength + kSebMultipartLength ) {
        if (error) {
          *error = QStringLiteral("Invalid multipart SEB header.");
        }
        return {};
      }

      qint64 firstPartLength = 0;
      memcpy( &firstPartLength                    // dest
            , raw.constData() + kSebPrefixLength  // src
            , sizeof(firstPartLength)             // n (bytes)
            );

      // Only use the first part!
      raw = raw.mid( kSebPrefixLength + kSebMultipartLength
                   , static_cast<int>(firstPartLength)
                   );
      
      if ( warnings ) {
        warnings->push_back(
            QStringLiteral("Ignored additional multipart SEB resources."));
      }
      continue;

    } else if ( prefix == kPrefixCustomHeader ) {

      if (raw.size() < kSebPrefixLength + kSebCustomHeaderLength) {
        if (error) {
          *error = QStringLiteral("Invalid custom-header SEB payload.");
        }
        return {};
      }

      qint32 headerLength = 0;
      memcpy( &headerLength
            , raw.constData() + kSebPrefixLength
            , sizeof(headerLength)
            );

      // Skip custom-header (keep the rest)
      raw = raw.mid( kSebPrefixLength + kSebCustomHeaderLength + headerLength );
      if (warnings) {
        warnings->push_back(
            QStringLiteral("Ignored unsupported custom SEB header data."));
      }
      continue;

    } else if ( prefix == kPrefixPlain /* plnd */ ) {

      // Found it! (skip the prefix)
      return raw.mid( kSebPrefixLength );

    } else if (  prefix == kPrefixPassword                 /* "pswd" */
              || prefix == kPrefixPasswordConfigureClient  /* "pwcc" */
              )
    {

      const bool hashPasswordBeforeUse =
        (prefix == kPrefixPasswordConfigureClient);

      const QByteArray encrypted = raw.mid( kSebPrefixLength ); // All the rest

      const int max_attempts = 5;
      for ( int attempt = 0; attempt < max_attempts; ++attempt ) {
        
        if ( !passwordProvider ) {
          if (error) {
            *error = QStringLiteral("This SEB file is encrypted and requires a password.");
          }
          return {};
        }

        const QString supplied = passwordProvider( hashPasswordBeforeUse );
        if ( supplied.isNull() || supplied.isEmpty() ) {
          if (error) {
            *error = QStringLiteral("Password entry was cancelled.");
          }
          return {};
        }

        const QString password = ( hashPasswordBeforeUse
                                 ? hashPassword(supplied).toUpper()
                                 : supplied
                                 );

        QByteArray decrypted;
        QString    decryptError;
        if ( decryptPasswordBlock( encrypted
                                 , password
                                 , &decrypted
                                 , &decryptError )
           )
        {
          raw = decrypted;
          if ( raw.size() >= 2 &&
               static_cast<unsigned char>(raw.at(0)) == 0x1f &&
               static_cast<unsigned char>(raw.at(1)) == 0x8b )
          {
            raw = inflateGzip( raw, &decryptError );
            if ( raw.isEmpty() ) {
              if (error) {
                *error = decryptError;
              }
              return {};
            }
          }
          break; // for( attempt )
        }

        if (attempt == (max_attempts-1)) {
          if (error) {
            *error = decryptError;
          }
          return {};
        }
      }  // for( attempt )

      continue;

    } else if (  prefix == kPrefixPublicKey           /* "pkhs" */
              || prefix == kPrefixPublicKeySymmetric  /* "phsk" */
              )
    {

      CertificateStore             store;
      PublicKeyEncryption          rsa(store); // Keeps non-const reference to store
      PasswordEncryption           aes;
      PublicKeySymmetricEncryption hybrid( store // non-const reference
                                         , rsa   // non-const reference
                                         , aes   // non-const reference
                                         );

      using LoadStatus = seb::configuration::contracts::LoadStatus;
      QSslCertificate cert;
      QByteArray      decrypted;
      LoadStatus      status;

      if (prefix == kPrefixPublicKey) {
        status = rsa.decrypt( raw.mid(kSebPrefixLength)
                            , decrypted  // non-const reference
                            , cert       // non-const reference
                            );
      } else {
        status = hybrid.decrypt( raw.mid(kSebPrefixLength)
                               , decrypted // non-const reference
                               , cert      // non-const reference
                               );
      }

      if ( status == LoadStatus::Success ) {
        raw = decrypted;
        continue;
      } else if (status == LoadStatus::PasswordNeeded) {
        if (error) {
          *error = QStringLiteral("A certificate with private key is required to open this file.");
        }
        return {};
      } else {
        if (error) {
          *error = QStringLiteral("Failed to decrypt certificate-encrypted SEB file.");
        }
        return {};
      }
    } else {
      // No known prefixes matched.
      break;
    }
  } // while(raw.size())

  return raw;
}

} // namespace seb::settingsinternal
