<?php
/**
 * @file
 * The video module has some hooks which should make adding
 * new features to the video module easier.
 *
 * This file contains example of implementation and documentation for
 * all the available hooks defined in the video module.
 *
 * Video module hooks are different from standard drupal hooks
 * Video module hooks have a leading "v_". The name of a function which
 * is implementing a video hook is something like: modulename_v_hookname
 *
 * Although each active module which implement a video module hooks
 * will be executed when that hook is called, if you are developing a
 * video module specific addition (a plug in) I suggest you to call your
 * module video_something and place it under your video module plugins folder.
 *
 * @author Fabio Varesano <fvaresano at yahoo dot it>
 * porting to Drupal 6
 * @author Heshan Wanigasooriya <heshan at heidisoft.com><heshanmw@gmail.com>
 * @todo
 */


//TODO: When we will release a stable version we have to document all the APIs
//      the video module have


function hook_v_info() {};


/**
 * This hook is called by the video_image plugins once
 * TODO: better documentation
*/
function hook_v_autothumbnail($node) {
  ;
}

/**
The hook_v_get_params is used by plugins to write an html param inside
inside video generated object tag during the play.

@param $node the node on which is being played

@return a keyed array of tipe 'param_name'=>'param_value'
*/
function hook_v_get_params(&$node) {
  return array('flashVars' => 'autostart=true&url=false');
}



