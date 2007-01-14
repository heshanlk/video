// $Id$
/**
 * @file
 * Javascript functions for busy status on video uploads
 * 
 * TODO: Support AJAX Uploads :-)
 *
 * @author Fabio Varesano <fvaresano at yahoo dot it>
*/

/**
 * Hide the node form and show the busy div
*/
Drupal.video_upload_hide = function () {
  $('#node-form').hide();
  $("#sending").show();
  $("#video_upload_cancel_link").click(Drupal.video_upload_show);
}

Drupal.video_upload_show = function() {
  $('#node-form').show();
  $("#sending").hide();
  
  //$("form").bind("submit", function() { return false; })
  window.location = window.location;
}

/**
 * Attaches the upload behaviour to the video upload form.
 */
Drupal.video_upload = function() {
  $('#node-form').submit(Drupal.video_upload_hide);
}

// Global killswitch
if (Drupal.jsEnabled) {
  $(document).ready(Drupal.video_upload);
}
