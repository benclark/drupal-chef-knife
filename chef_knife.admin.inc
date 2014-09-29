<?php

/**
 * @file
 * Administrative functions and callbacks for chef_knife module.
 */


/**
 * Admin settings form for chef_knife.
 */
function chef_knife_settings_form() {
  $form = array();

  $form['chef_knife_server'] = array(
    '#type' => 'textfield',
    '#title' => t('Chef server URI'),
    '#description' => t('The URI of the Chef server, including organization.'),
    '#default_value' => variable_get('chef_knife_server', 'https://api.opscode.com/organizations/org_name'),
    '#required' => TRUE,
  );
  $form['chef_knife_client_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Chef client name'),
    '#description' => t('The client name that will correspond to the private key.'),
    '#default_value' => variable_get('chef_knife_client_name', ''),
    '#required' => TRUE,
  );
  $form['chef_knife_private_key_file'] = array(
    '#type' => 'textfield',
    '#title' => t('Private key file path'),
    '#description' => t('Absolute path to the private key file for the client. Must be readable by the PHP process.'),
    '#default_value' => variable_get('chef_knife_private_key_file', ''),
    '#required' => TRUE,
  );

  return system_settings_form($form);
}

/**
 * Validate callback for the settings form.
 */
function chef_knife_settings_form_validate($form, &$form_state) {
  // Try connecting to the server.
  try {
    $chef_server = $form_state['values']['chef_knife_server'];
    $client_name = $form_state['values']['chef_knife_client_name'];
    $private_key_file = $form_state['values']['chef_knife_private_key_file'];
    // Use the 'users' endpoint to prove that we can grab data from the server.
    $result = _chef_knife('nodes', 'GET', NULL, $chef_server, $client_name, $private_key_file);
  }
  catch (Exception $e) {
    form_set_error('chef_knife_server', t('Could not authenticate with Chef server with the provided credentials.'));
  }
}

/**
 * Raw access to the Chef server API.
 */
function chef_knife_test() {
  $form = array();

  $form['endpoint'] = array(
    '#type' => 'textfield',
    '#title' => t('Endpoint'),
    '#required' => TRUE,
  );
  $form['method'] = array(
    '#type' => 'select',
    '#title' => t('HTTP Method'),
    '#options' => array(
      'GET' => 'GET',
    ),
    '#default_value' => 'GET',
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * Submit callback for the Chef server API test form.
 */
function chef_knife_test_submit($form, &$form_state) {
  if (!empty($form_state['values']['endpoint'])) {
    $result = chef_knife($form_state['values']['endpoint'], $form_state['values']['method']);
    if (module_exists('devel')) {
      dpm($result);
    }
    else {
      drupal_set_message(t('To view the test results, please enable the devel module.'));
    }
  }
  drupal_set_message(t('Chef server contacted.'));
}
