<?php

class ChefKnifeAPIAuth implements HttpClientAuthentication {

  private $mandatory_headers;
  private $client_name;
  private $private_key_file;

  public function __construct($client_name = NULL, $private_key_file = NULL) {
    $this->client_name = $client_name;
    $this->private_key_file = $private_key_file;

    // Set some mandatory headers.
    $this->mandatory_headers = array(
      'X-Ops-UserId' => $client_name,
      'X-Ops-Sign' => 'version=1.0',
      'X-Chef-Version' => CHEF_KNIFE_API_VERSION,
      // Format the timestamp in ISO 8601
      'X-Ops-Timestamp' => gmdate('Y-m-d\TG:i:s\Z'),
    );
  }

  /**
   * Hash a string using SHA1.
   */
  private function hash($data) {
    return sha1($data, TRUE);
  }

  /**
   * Encode a string using Base64, matching RFC 2045 semantics.
   */
  private function base64($data, $chunk_split = TRUE, $chunklen = 76) {
    $data64 = base64_encode($data);
    return ($chunk_split) ? trim(chunk_split($data64, $chunklen)) : $data64;
  }

  /**
   * Sign a string using the private_key, and return the base64 encoded result.
   */
  private function sign($data) {
    if ($pkeyid = openssl_pkey_get_private('file://' . $this->private_key_file)) {
      openssl_private_encrypt($data, $signature, $pkeyid);
      openssl_pkey_free($pkeyid);
    }
    else {
      throw new HttpClientException('Unable to read private key file');
    }
    return isset($signature) ? $this->base64($signature, TRUE, 60) : '';
  }

  /**
   * Authenticate method (to be used by HttpClient).
   *
   * @see HttpClientAuthentication::authenticate()
   */
  public function authenticate($request) {
    // Add our mandatory headers to the request.
    foreach ($this->mandatory_headers as $mandatory_header => $value) {
      if (!isset($request->headers[$mandatory_header])) {
        $request->setHeader($mandatory_header, $value);
      }
    }

    // Find the endpoint.
    $endpoint = '';
    if (preg_match('/organizations\/(.+)\/(.+)/', $request->url, $matches)) {
      $endpoint = '/organizations/' . $matches[1] . '/' . $matches[2];
    }

    // Hash the path.
    $hashed_path = $this->base64($this->hash($endpoint));

    // Hash the data payload.
    $hashed_body = $this->base64($this->hash($request->data));
    $request->setHeader('X-Ops-Content-Hash', $hashed_body);

    // Build the canonical request.
    $canonical_request = array(
      "Method:{$request->method}",
      "Hashed Path:$hashed_path",
      "X-Ops-Content-Hash:$hashed_body",
      "X-Ops-Timestamp:{$request->headers['X-Ops-Timestamp'][0]}",
      "X-Ops-UserId:{$this->client_name}",
    );
    $canonical_request_string = implode($canonical_request, "\n");

    // Sign the canonical request.
    $auth_headers_string = $this->sign($canonical_request_string);

    // Split the signed request into multiple headers.
    if (!empty($auth_headers_string)) {
      $i = 0;
      foreach (explode("\r\n", $auth_headers_string) as $auth_header) {
        $request->setHeader('X-Ops-Authorization-' . ++$i, $auth_header);
      }
    }
  }
}
