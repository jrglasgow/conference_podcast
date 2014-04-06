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

/**
 * submit handler
 */
function conference_podcast_admin_submit($form, &$form_state) {
  dpm($form_state['values'], 'values');
  if ($form_state['values']['delete']) {
    // truncate the table
    db_truncate('conference_podcast')->execute();
  }
  $return = drupal_http_request($form_state['values']['import_url']);
  $html = $return->data;
  htmlqp($html, 'table.sessions tr')->each('conference_podcast_process_item');
}

function conference_podcast_process_item($count, $DOMObject) {
  // check to see if this has a speaker
  if (qp($DOMObject, 'span.speaker')->length) {
    $fields = array();
    $fields['title'] = qp($DOMObject, 'span.talk')->text();

    $fields['speaker'] = trim(str_replace('By ', '', qp($DOMObject, 'span.speaker')->text()));
    $fields['url'] = qp($DOMObject, 'a.audio-mp3')->attr('href');
    $talk_id = explode('/',$fields['url']);
    if (isset($talk_id[5])) {
      $fields['conference'] = ucwords(str_replace('-', ' ', $talk_id[5]));
    }
    else {
      $fields['conference'] = t('Not Available');
    }
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
  // get the last added episode of the podcast
  $result = db_select('conference_podcast', 'cp')
    ->fields('cp')
    ->orderBy('last_played', 'DESC')
    ->range(0, 1)
    ->execute()
    ->fetchAssoc();

  if ($result['last_played'] < (REQUEST_TIME - 60 * 60 * 24)) {
    // it has been more than 24 hours since the last episode was added, add another episode
    // get the new episode
    $new_episode = db_select('conference_podcast', 'cp')
      ->fields('cp')
      ->orderBy('last_played', 'ASC')
      ->orderBy('talk_id')
      ->range(0, 1)
      ->execute()
      ->fetchAssoc();

    // update the last_played column for the new episode
    db_update('conference_podcast')
      ->fields(array(
        'last_played' => REQUEST_TIME,
      ))
      ->condition('talk_id', $new_episode['talk_id'])
      ->execute();
  }

  $query = db_select('conference_podcast', 'cp')
    ->fields('cp')
    ->condition('last_played', 0, '<>')
    ->orderBy('last_played', 'DESC');

  $result = $query->execute();
  $episodes = $result->fetchAllAssoc('talk_id');
  if (empty($episodes)) {
    return;
  }

  return conference_podcast_feed($episodes);
}
/**
 * Provide an RSS feed for a returned asset list
 */
function conference_podcast_feed($episodes) {
  $feed = array(
    0 => array(
      'key'         => 'rss',
      'attributes'  => array(
        'xmlns:itunes' => 'http://www.itunes.com/dtds/podcast-1.0.dtd',
        'xmlns:atom'    => 'http://www.w3.org/2005/Atom',
        'xmlns:media'   => 'http://search.yahoo.com/mrss/',
        'xmlns:dc'      => 'http://purl.org/dc/elements/1.1/',
        'version'   => '2.0',
      ),
      'value' => array(
        'channel'   => array(
          'title'         => t('LDS General Conference'),
          'link'          => url(str_replace('/rss.xml', '', $_GET['q']), array('absolute' => TRUE)),
          'language'      => 'en-us',
          'copyright'     => t('© @year LDS', array('@year' => date('Y', REQUEST_TIME))),
          'description'   => t('General Conference Addresses'),
          //'itunes:category' => 'Religion',
          'itunes:owner'    => array(
            'itunes:name'     => 'James Glasgow',
            'itunes:email'    => 'james@glasgowclan.com',
          ),
          /*
          'itunes:subtitle' => '',
          'itunes:author'   => '',
          'itunes:summary'  => '',
          */
          'itunes:explicit' => 'no',
          /*0  => array(
            'key'             => 'itunes:image',
            'attributes'      => array(
              'href'            => $album['cover']['url'],
            ),
          ),*/
        ),
      ),
    ),
  );
  $feed[0]['value']['channel'][] = array(
    'key'         => 'atom:link',
    'attributes'  => array(
      'href'        => url($_GET['q'], array('absolute' => TRUE)),
      'rel'         => 'self',
      'type'        => 'application/rss+xml',
    ),
  );

  foreach ($episodes AS $episode) {
    $item = conference_podcast_feed_item($episode);
    $feed[0]['value']['channel'][] = $item;
  }

  drupal_add_http_header('Content-Type', 'application/xml');

  print '<?xml version="1.0" encoding="UTF-8" ?>' . "\n" . format_xml_elements($feed);
}

/**
 * when building RSS feeds create the item
 */
function conference_podcast_feed_item($episode) {
  $item = array(
    'key'       => 'item',
    'value'     => array(
      'title'           => $episode->title ? $episode->title : t('title yet to come'),
      'itunes:author'   => $episode->speaker,
      //'description'     => '',
      //'itunes:duration' => $asset['running_time'],
      'itunes:explicit' => 'clean',
      array(
        'key'         => 'guid',
        'value'       => $episode->talk_id .'-'. date('r',$episode->last_played),
        'attributes'  => array(
          'isPermaLink' => 'false',
        ),
      ),
      'link'        => $episode->url,
      'pubDate'     => date('r', $episode->last_played),
    ),
  );
  $item['value'][] = array(
    'key'         => 'enclosure',
    'value'       => '',
    'attributes'  => array(
      'url'         => $episode->url,
      'length'      => $episode->size,
      'type'        => file_get_mimetype($episode->url),
    ),
  );
  return $item;
}
