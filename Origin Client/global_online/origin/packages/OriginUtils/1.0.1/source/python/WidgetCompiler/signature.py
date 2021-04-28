try:
    from Crypto.Hash import SHA256
    from Crypto.Signature import PKCS1_v1_5
except:
    pass

import base64

from datasource import StringDataSource

SIGNATURE_PACKAGE_PATH = 'origin/signature'

class PackageCryptographicSignature(object):
    """Class to cryptographically sign a widget package`"""
    def __init__(self, private_key):
        self.private_key = private_key
        self.signatures = {}

    def _sign_string(self, string):
        """Signs a string with PKCS1 v1.5 and SHA256 with our private key"""
        # Build our SHA-256 hash
        file_hash = SHA256.new(string)

        # Sign the hash
        return PKCS1_v1_5.new(self.private_key).sign(file_hash)
        
    def add_signed_file(self, package_path, data_source):
        # Load the data in
        file_data = data_source.read()

        # Make our signature
        file_digest = SHA256.new(file_data).hexdigest()
        self.signatures[package_path.encode('utf-8')] = file_digest

    def package_signature(self):
        output_string = "originsigv1\n"

        # Signatures have to be ordered to be canonical
        for package_path in sorted(self.signatures.iterkeys()):
            output_string += self.signatures[package_path]
            output_string += "\t"
            output_string += package_path + "\n"
        
        # Sign what we have so far
        partial_signature = self._sign_string(output_string)

        output_string += base64.standard_b64encode(partial_signature) + "\n"
        
        return StringDataSource(output_string)
