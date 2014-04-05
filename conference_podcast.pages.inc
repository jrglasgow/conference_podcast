<?php

/**
 * @file
 *
 * page callbacks
 */

/**
 * administration form
 */
function conference_podcast_admin() {
  $form = array();

  $form['import_url'] = array(
    '#type'         => 'textfield',
    '#title'        => t('Import URL'),
    '#description'  => t('The URL to import the conference information from'),
  );
  $form['delete'] = array(
    '#type'         => 'checkbox',
    '#title'        => t('Delete'),
    '#description'  => t('Delete all episodes and start from scratch.'),
  );
  $form['submit'] = array(
    '#type'         => 'submit',
    '#value'        => t('Import'),
  );


  return $form;
}

function conference_podcast_admin_submit($form, &$form_state) {
  $return = drupal_http_request($form_state['values']['import_url']);
  $html = $return->data;
  htmlqp($html, 'table.sessions tr')->each('conference_podcast_process_item');
}
function conference_podcast_process_item($count, $DOMObject) {
  // check to see if this has a speaker
  if (qp($DOMObject, 'span.speaker')->length) {
    //dpm(qp($DOMObject)->html(),'$DOMObject');
    $fields = array();
    $fields['title'] = qp($DOMObject, 'span.talk')->text();

    $fields['speaker'] = trim(str_replace('By ', '', qp($DOMObject, 'span.speaker')->text()));
    $fields['url'] = qp($DOMObject, 'a.audio-mp3')->attr('href');
    $talk_id = explode('/',$fields['url']);
    $fields['conference'] = ucwords(str_replace('-', ' ', $talk_id[5]));
    $talk_id = explode('-',array_pop($talk_id));
    if (empty($talk_id[0])) {
      return;
    }
    $fields['talk_id'] = $talk_id[0] . '-' . $talk_id[1] . '-' . $talk_id[2];
    $fields['last_played'] = 0;
    $headers = cp_remote_file_headers($fields['url']);
    if (isset($headers['Content-Length'])) {
      $fields['size'] = $headers['Content-Length'];
    }
    //dpm($headers, '$headers');
    //dpm($fields, '$fields');

    $query = db_merge('conference_podcast')
      ->key(array('talk_id' => $fields['talk_id']))
      ->fields($fields)
      ->execute();
  }
}

/**
 * generate the podcast feed
 */
function conference_podcast_podcast_feed() {
  // get the newest podcast
  $query = db_select('conference_podcast', 'cp')
    ->fields();

}
