<?php

/**
 * @file
 * XlmsDevice editing UI.
 */

/**
 * Generates the device type editing form.
 */
function xlms_device_overview() {
  $devices = xlms_device_load_all();
  $header = array();

  $rows = array();
  foreach ($devices as $device) {
    $rows[] = array(
      l($device->name, $device->defaultUri()['path']),
    );
  }
  $empty = t('No devices configured.');
  return theme('table', array('header' => $header, 'rows' => $rows, 'empty' => $empty));
}

/**
 * UI controller.
 */
class XlmsDeviceUIController extends EntityDefaultUIController {

  /**
   * Overrides hook_menu() defaults.
   */
  public function hook_menu() {
    $items = parent::hook_menu();
		$items[$this->path]['description'] = 'Manage devices.';
    return $items;
  }
}

/**
 * Generates the device type editing form.
 */
function xlms_device_form($form, &$form_state, $xlms_device, $op = 'edit', $entity_type = NULL) {
  if ($op == 'clone') {
    // Only label is provided for cloned entities.
    $xlms_device->label .= ' (cloned)';
    $xlms_device->type = $entity_type . '_clone';
  }

  $form['configuration_file'] = array(
    '#title' => t('Configuration file'),
    '#type' => 'managed_file',
    '#required' => TRUE,
    '#description' => t('A YAML file that defines this machine\'s configuration parameters and metrics.'),
    '#default_value' => $xlms_device->configurationFile()->fid,
    '#upload_location' => 'public://devices/',
    "#upload_validators"  => array("file_validate_extensions" => array("yaml")),
  );

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => isset($xlms_device->label) ? $xlms_device->label : '',
  );
  // Machine-readable type name.
  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($xlms_device->type) ? $xlms_device->type : '',
    '#machine_name' => array(
      'exists' => 'xlms_device_load_name',
      'source' => array('label'),
    ),
    '#description' => t('A unique machine-readable name for this device. It must only contain lowercase letters, numbers, and underscores.'),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save device'),
    '#weight' => 40,
  );
  return $form;
}

/**
 * Form API submit callback for the type form.
 */
function xlms_device_form_submit(&$form, &$form_state) {
  $device = entity_ui_form_submit_build_entity($form, $form_state);

  // Save file and document its usage here. 
  // See https://api.drupal.org/api/drupal/developer!topics!forms_api_reference.html/7#managed_file
  $file = file_load($form_state['values']['configuration_file']);
  $file->status = FILE_STATUS_PERMANENT;
  file_save($file);

  $device->setConfigurationFile($file);

  // Save device.
  $device->save();

  file_usage_add($file, 'xlms_device', 'xlms_device', $device->did);  

  $form_state['redirect'] = 'admin/config/xlms/devices';
}

