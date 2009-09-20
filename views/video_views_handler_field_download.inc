<?php
//$Id$
/**
* Implementation of hook_views_tables
*
* @return
*   array - Enables support in the video module for views integration
 * @author Glen Marianko Twitter@demoforum <glenm at demoforum dot com>
 * @todo
**/

/**
 * Field handler to display the play length of the video.
 *
 * @ingroup views_field_handlers
 */
class video_views_handler_field_download extends views_handler_field {
  /**
   * Render field output to the browser.
   */
  function render($values) {
    return _video_views_handler_field_download($values);
  }
}
/**
* Handler to to return the correct download link for the video in a field
**/
function _video_views_handler_field_download($values) {
  /* $str = NULL;
   foreach($values as $key => $value) {
        $str .= $key .'<br>';
   }
   return $str; */
  if($values->node_type &&  $values->node_type != 'video') return NULL;
  return l(t('Download'), "node/$values->nid/download", array());
}

