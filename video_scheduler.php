<?php
//$Id$
/**
 * @file
 * Implement video rendering scheduling.
 * If you are not using sites/default/settings.php as your settings file,
 * add an optional parameter for the drupal site url:
 * "php video_scheduler.php http://example.com/" or
 * "php video_scheduler.php http://example.org/drupal/"
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft dot com, heshanmw at gmail dot com>
 *
 */

if (isset($_SERVER['argv'][1])) {
  $url = parse_url($_SERVER['argv'][1]);
  $_SERVER['SCRIPT_NAME'] = $url['path'];
  $_SERVER['HTTP_HOST'] = $url['host'];
}

include_once('./includes/bootstrap.inc');
// let's bootstrap: we will be able to use drupal apis
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

//include our conversion class (also contains our defines)
module_load_include('inc', 'video', 'includes/conversion');
$video_conversion = new video_conversion;
$video_conversion->run_queue();
?>