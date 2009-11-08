<?php
//$Id$
/**
 * @file
 * Some functions for using video hosting providers api (Youtube, Google Video, etc..)
 * Part of this code has been inspired by the video_cck module and adapted
 * for the video module by jyamada1
 *
 * @author Fabio Varesano <fvaresano at yahoo dot it>
 * @author Heshan Wanigasooriya <heshan at heidisoft.com><heshanmw at gmail dot com>
 * @todo
 */

// Upload token url.
define('VIDEO_UPLOAD_GDATA_TOKEN_UPLOAD_URL', 'http://gdata.youtube.com/action/GetUploadToken');

// Note the different schemes for categories and developer tags.
define('VIDEO_UPLOAD_GDATA_CATEGORIES_SCHEME', 'http://gdata.youtube.com/schemas/2007/categories.cat');
define('VIDEO_UPLOAD_GDATA_DEVELOPER_TAG_SCHEME', 'http://gdata.youtube.com/schemas/2007/developertags.cat');

// Authentication url.
define('VIDEO_UPLOAD_GDATA_AUTHENTICATE_URL', 'https://www.google.com/youtube/accounts/ClientLogin');


 /**
 * Authenticate YouTube user account.
 */
function _video_upload_gdata_authenticate_youtube() {
  // Username.
  // $username = variable_get('video_upload_youtube_username', FALSE);
  $username = 'heshanmw';
  if (!$username) {
    watchdog('video_upload', 'No YouTube username set', array(), WATCHDOG_ERROR);
  }

  // Password.
  // $password = variable_get('video_upload_youtube_password', FALSE);
  $password = 'chicagobulls';
  if (!$password) {
    watchdog('video_upload', 'No YouTube password set', array(), WATCHDOG_ERROR);
  }

  if (!$username || !$password) {
    drupal_set_message(t('YouTube Uploads not currently available'), 'error');
    return FALSE;
  }

  if (_video_apiclient_youtube_init('authenticate')) {
    try {
      $http_client = Zend_Gdata_ClientLogin::getHttpClient(
      $username, // username
      $password, // password
      $service = 'youtube',
      $client = NULL,
      $source = 'drupal',
      $loginToken = NULL,
      $loginCaptcha = NULL,
      VIDEO_UPLOAD_GDATA_AUTHENTICATE_URL
      );
    }
    catch (Zend_Gdata_App_AuthException $authEx) {
      drupal_set_message(t('YouTube uploads currently unavailable'), 'error');
      watchdog('video_upload', 'Authentication error for YouTube Account', array(), WATCHDOG_ERROR);
      return FALSE;
    }
    catch (Zend_Gdata_App_HttpException $e) {
      drupal_set_message(t('YouTube uploads currently unavailable'), 'error');
      watchdog('video_upload', 'Authentication error for YouTube Account: %error', array('%error' => $e->getMessage()), WATCHDOG_ERROR);
      return FALSE;
    }
  }
  else {
    return FALSE;
  }

  // Pass developer key.
  // $developer_key = variable_get('video_upload_youtube_developer_key', FALSE);
  $developer_key = variable_get('video_youtube_api_key', '');
  if (!$developer_key) {
    watchdog('video_upload', 'No developer key set', array(), WATCHDOG_ERROR);
    drupal_set_message(t('YouTube Uploads not currently available'), 'error');
    return FALSE;
  }
  $http_client->setHeaders('X-GData-Key', 'key=' . $developer_key);

  return $http_client;
}

/*
 *  Create a Zend YouTube object.
 * @param mixed $http_client
 *   Either a boolean (if FALSE, an un-authenticated connection is
 *   made), or an authentication object.
 * @return object Zend YouTube object
 */
function _video_apiclient_youtube_gdata($http_client = NULL) {
  _video_apiclient_youtube_init('youtube');
  try {
    $yt = new Zend_Gdata_YouTube($http_client);
    // Set API version to 2.0.
    $yt->setMajorProtocolVersion(2);
  }
  catch (Zend_Gdata_App_HttpException $e) {
    watchdog('youtube_video', 'Authentication error while creating a YouTube connection object: %error', array('%error' => $e->getMessage()), WATCHDOG_ERROR);
  }
  return $yt;
}

function _video_apiclient_youtube_video_entry($id, $cacheable = TRUE) {
  //_video_apiclient_youtube_init('youtube');
  // Get new http authentication.
  $http_client = _video_upload_gdata_authenticate_youtube();
  //initialize Gdata lib
  $yt = _video_apiclient_youtube_gdata($http_client);
  //$yt = new Zend_Gdata_YouTube($http_client);
  // set API version
  //$yt->setMajorProtocolVersion(2);
  // if it's a cachable request, try to load a cached value
//  if ($cacheable) {
//    if ($cache = cache_get($id, 'cache')) {
//      return unserialize($cache->data);
//    }
//  }
  try{
    //get video entried
    $videoEntry = $yt->getVideoEntry($id);
    watchdog('youtube_video', $videoEntry->getVideoDuration().$videoEntry->getVideoId().$videoEntry->getVideoTitle());
//    print_r($videoEntry);
//    exit;
  }
  catch (Zend_Gdata_App_HttpException $e){
    watchdog('youtube_video', 'Authentication error while receiving YouTube connection object: %error', array('%error' => $e->getMessage()), WATCHDOG_ERROR);
  }
  // save a cacheable result for future use
//  if ($cacheable) {
//    cache_set($id, serialize($videoEntry), 'cache', time() + 3600);
//  }
  return $videoEntry;
}

/**
 * function to include necessary Zend Gdata libraries.
 */
function _video_apiclient_youtube_init($op) {
  // Include Zend loader class.
  _video_upload_gdata_set_include_path();
  //TODO :  move Zend to the library folder
  if (@include_once('Zend/Loader.php')) {
    switch ($op) {
    case 'authenticate' :
      Zend_Loader::loadClass('Zend_Gdata_ClientLogin');
      break;
    case 'youtube' :
      Zend_Loader::loadClass('Zend_Gdata_YouTube');
      break;
    }
    return TRUE;
  }
  else {
    watchdog('youtbe_video', "Couldn't find the Zend client libraries.", array(), WATCHDOG_ERROR);
  }
}

/**
 * Set include path for Zend GData client libraries, which should be
 * installed locally in the module directory.
 */
function _video_upload_gdata_set_include_path() {
  // Include path must be modified.
  static $path;
  if (!$path) {
    $path = get_include_path();
    set_include_path($path . PATH_SEPARATOR . drupal_get_path('module', 'video') . '/includes/youtube');
  }
}

/**
 * When an include file requires to read an xml to receive information, such as for thumbnails,
 * this script can be used to request the xml and return it as an array.
 * Note that this is a modified function from the flickr.module, made to handle this type of
 * call more generically. also, i suspect it could be done easier (and more quickly) in php 5.
 *   @param $provider
 *     the string of the third party provider, such as 'youtube' or 'google'
 *   @param $url
 *     the url for the xml request
 *   @param $args
 *     an array of args to pass to the xml url
 *   @param $cacheable
 *     optional; if true, the result of this xml request will be cached. good to play nice w/
 *     the third party folks so they don't stop providing service to your site...
 *   @return
 *     the xml results returned as an array
 */
function _video_apiclient_request_xml($provider, $url, $args = array(), $cacheable = true) {
  ksort($args);

  // build an argument hash that we'll use for the cache id and api signing
  $arghash = $provider . ':';
  foreach($args as $k => $v) {
    $arghash .= $k . $v;
  }

  // build the url
  foreach ($args as $k => $v) {
    $encoded_params[] = urlencode($k).'='.urlencode($v);
  }
  $url .= '?'. implode('&', $encoded_params);

  // if it's a cachable request, try to load a cached value
  if ($cacheable) {
    if ($cache = cache_get($arghash, 'cache')) {
      return unserialize($cache->data);
    }
  }

  // connect and fetch a value
  $result = drupal_http_request($url);

  if ($result->code == 200) {
    $parser = drupal_xml_parser_create($result->data);
    $vals = array();
    $index = array();
    xml_parse_into_struct($parser, $result->data, $vals, $index);
    xml_parser_free($parser);

    $params = array();
    $level = array();
    $start_level = 1;
    foreach ($vals as $xml_elem) {
      if ($xml_elem['type'] == 'open') {
        if (array_key_exists('attributes',$xml_elem)) {
          list($level[$xml_elem['level']],$extra) = array_values($xml_elem['attributes']);
        } else {
          $level[$xml_elem['level']] = $xml_elem['tag'];
        }
      }
      if ($xml_elem['type'] == 'complete') {
        $php_stmt = '$params';
        while($start_level < $xml_elem['level']) {
          $php_stmt .= '[$level['.$start_level.']]';
          $start_level ++;
        }
        $php_stmt .= '[$xml_elem[\'tag\']][] = $xml_elem[\'value\'];';
        eval($php_stmt);
        $start_level--;
      }
    }

    // save a cacheable result for future use
    if ($cacheable) {
      cache_set($arghash, serialize($params), 'cache', time() + 3600);
    }
    return $params;
  }
  return array();
}


/**
 * Create a file object from thumbnail images from providers
 *  to allow for automatic thumbnailing of videos from providers
 *  @param $node
 *    the video node being called
 *  @return
 *    a file object containing the thumbnail file
 */
/*
function _video_apiclient_provider_auto_thumbnail($node) {
 // get thumbnail url
 if(_video_get_filetype($node->vidfile) == 'youtube') {
   $thumbnail = _video_apiclient_youtube_thumbnail($node->vidfile);
 }
 else {
   $thumbnail = _video_apiclient_google_thumbnail($node->vidfile);
 }

 // save image to temp directory for processing
 $image = image_gd_open($thumbnail, 'jpeg');
 $location = file_directory_temp() .'/'. $node->vidfile .'.jpg';
 image_gd_close($image, $location, 'jpeg');


 // get info and build a file object
 $filepath = file_create_path($location, file_directory_temp());
 $info = image_get_info($filepath);

 $file = new stdClass();
 $file->filepath = realpath($filepath);
 $file->filename = basename($file->filepath);
 $file->filesize = $info['file_size'];
 $file->filemime = $info['mime_type'];

 return $file;
}
*/

