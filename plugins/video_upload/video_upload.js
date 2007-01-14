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
}

/**
 * Attaches the upload behaviour to the video upload form.
 */
Drupal.video_upload = function() {
  $('#node-form').submit(video_upload_hide);
}

// Global killswitch
if (Drupal.jsEnabled) {
  $(document).ready(Drupal.video_upload);
}
