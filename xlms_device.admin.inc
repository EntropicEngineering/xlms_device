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
    $uri = entity_uri('xlms_device', $device);
    $rows[] = array(
      l($device->name, $uri['path'], $uri['options']),
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
 * Generates the device editing form.
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
    '#description' => t('A zip file containing assets and/or A YAML file that defines this machine\'s configuration parameters and metrics.'),
    '#default_value' => $xlms_device->configurationFile()->fid,
    '#upload_location' => 'public://devices/',
    "#upload_validators"  => array("file_validate_extensions" => array("zip", "yaml")),
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

  if ($archiver = archiver_get_archiver($file->uri)) {

    // Validate contents
    $files = array();
    $dirname = NULL;
    $list = $archiver->listContents();

    // Drop everything into the temp directory.
    $archiver->extract('temporary://');
    $base_uri = 'public://devices/' . $form_state['values']['name'] .'/';
    $asset_uri = $base_uri . 'assets/';
    drupal_mkdir($asset_uri, NULL, TRUE);

    foreach ($list as $item) {
      if (!isset($dirname) && ($dir = dirname($item))) {
        if ($dir != '.') {
          $dirname = $dir;
        }
      }
      $name = basename($item);

      // Do not include dotfiles.
      if ($name[0] == '.') continue;

      $data = file_get_contents('temporary://' . $item);

      // Separate YAML file.
      $extension = strtolower(end(explode('.', $name)));
      if ($extension == 'yaml') {
        $yaml = file_save_data($data, $base_uri . $name);
        $device->setConfigurationFile($yaml);
        $files[] = $yaml;
      }
      else {
        $files[] = file_save_data($data, $asset_uri . $name);
      }
    }
  }

  // It's just a YAML file?
  else {
    $extension = strtolower(end(explode('.', $file->filename)));

    if ($extension == 'yaml') {
      $device->setConfigurationFile($file);
    }
  }

  $interface = $device->configuration('interface');
  foreach ($files as $item) {
    if ($item->filename == $interface) {
      $device->setInterfaceFile($item);
      break;
    }
  }

  // Save device.
  $device->save();

  foreach ($files as $item) {
    file_usage_add($item, 'xlms_device', 'xlms_device', $device->did);  
  }

  $form_state['redirect'] = 'admin/xlms/devices';
}

