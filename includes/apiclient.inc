<?php
/**
 * @file
 * Some functions for using video hosting providers api (Youtube, Google Video, etc..)
 * Part of this code has been inspired by the video_cck module and adapted
 * for the video module by jyamada1
 *
 * @author Fabio Varesano <fvaresano at yahoo dot it>
 * porting to Drupal 6
 * @author Heshan Wanigasooriya <heshan at heidisoft.com><heshanmw@gmail.com>
 * @todo
 */


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
 foreach($args as $k => $v){
   $arghash .= $k . $v;
 }

 // build the url
 foreach ($args as $k => $v){
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
     cache_set($arghash, 'cache', time() + 3600, serialize($params));
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

