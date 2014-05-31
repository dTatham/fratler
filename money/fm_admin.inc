<?php
/**
 * @file
 * Admin-Section
 */

/**
 * Page callback: fratler_money settings
 *
 * @see fratler_money_menu()
 */
function fratler_money_admin_form($form, &$form_state) {
	if (!function_exists("_fm_strip"))
		module_load_include('inc', 'fratler_money', 'fm_tools');

	$form['general'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('General Settings'),
	);

	// Try to automagically determine the available locales
	$locales = false;
	exec('test "x$(which locale)" != "x" && locale -a', $locales);		// Thanks to ChrisH of #debian - I know shit about bash-scripting ;D

	if (empty($locales)) 		// Apparently, we're not working on a linux-machine. Don't self destruct just yet...
		$form['general']['fm_locale'] = array(
			'#type'		=> 'textfield',
			'#title'	=> t('Used locale'),
			'#default_value' => variable_get('fm_locale', 'C'),
			'#size'		=> 10,
			'#maxlength'	=> 30,
			'#description'	=> t('Please enter a locale-String you would like to use to format the amounts.<br />Enter "C" as fallback.'),
			'#required'	=> true,
			'#field_suffix'	=> t('Currently using "@locale" (example: @num)', array('@locale' => variable_get('fm_locale', 'C'), '@num' => money_format('%n', -1234567.890))),
			'#suffix'	=> !empty($locales) ? t('Available locales:') . '<pre>' . implode(', ', $locales) . '</pre>' : t('Run "locale -a" (or your system\'s equivalent) on the host to determine which locales are available.'),
		);
	else
		$form['general']['fm_locale'] = array(
			'#type'		=> 'select',
			'#title'	=> t('Used locale'),
			'#default_value' => variable_get('fm_locale', 'C'),
			'#size'		=> 1,
			'#options'	=> drupal_map_assoc($locales),
			'#description'	=> t('Please enter a locale-String you would like to use to format the amounts.<br />Enter "C" as fallback.'),
			'#required'	=> true,
			'#field_suffix' => t('Currently using "@locale" (example: @num)', array('@locale' => variable_get('fm_locale', 'C'), '@num' => money_format('%n', -1234567.890))),
	);

	$form['general']['fm_pager'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Statement entries per page'),
		'#description'	=> t('How many statement-records would you like to display per page?'),
		'#size'		=> 5,
		'#maxlength'	=> 3,
		'#default_value' => variable_get('fm_pager', 10),
	);

	$locales = localeconv();
	$form['transfer'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('Transfer-related settings'),
	);
	$form['transfer']['fm_trans_max'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Maximum transaction amount'),
		'#description'	=> t('The highest amount that can be transferred in one transaction. <br />Enter 0 for unlimited transactions.'),
		'#default_value' => money_format("%!n", variable_get('fm_trans_max', 10)),
		'#size'		=> 6,
		'#field_prefix'	=> $locales['p_cs_precedes'] ? _fm_cs() : '',
		'#field_suffix'	=> !$locales['p_cs_precedes'] ? _fm_cs() : '',
	);
	$form['transfer']['fm_overdraw_limit'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Overdraw limit'),
		'#description'	=> t('The amount a normal user may overdraw its amount by. (Enter a positive value to keep a "safety-deposit")'),
		'#default_value' => money_format("%!n", variable_get('fm_overdraw_limit', 0)),
		'#size'		=> 6,
		'#field_prefix'	=> $locales['p_cs_precedes'] ? _fm_cs() : '',
		'#field_suffix'	=> !$locales['p_cs_precedes'] ? _fm_cs() : '',
	);

	$form['transfer']['fm_transfer_header'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('User roles'),
		'#description'	=> t('Select the user-roles which may participate in transfers (You can use this to exclude certain user-groups). <br /><b>Note:</b> This is only effictive for the recipient side. You can set exceptions in !settings.', array('!settings' => l(t('Permissions'), 'admin/people/permissions'))),
		'#collapsible'	=> true,
		'#collapsed'	=> false,
	);
	$roles = array();
	foreach (user_roles(true) as $rid => $role)
		$roles[$rid] = array(t(check_plain($role)));

	$form['transfer']['fm_transfer_header']['fm_transfer_groups'] = array(
		'#type'		=> 'tableselect',
		'#header'	=> array(t('Name')),
		'#options'	=> $roles,
		'#default_value' => variable_get('fm_transfer_groups', array()),
		'#empty'	=> t('No roles defined'),
	);


	return system_settings_form($form);
}

/**
 * implements form_validate()
 */
function fratler_money_admin_form_validate($form, &$form_state) {
	module_load_include('inc', 'fratler_money', 'fm_tools');

	if (!is_numeric(_fm_strip($form_state['values']['fm_trans_max'])) || _fm_parse_float($form_state['values']['fm_trans_max']) < 0 ) 
		form_set_error('fm_trans_max', t(':amount invalid', array(':amount' => t('Maximum transaction amount'))));
	
	if (!is_numeric(_fm_strip($form_state['values']['fm_overdraw_limit'])))
		form_set_error('fm_trans_max', t(':amount invalid', array(':amount' => t('Maximum transaction amount'))));
	if (!is_numeric($form_state['values']['fm_pager']))
		form_set_error('fm_pager', t('Please enter a numerical value.'));

	form_set_value(array('#parents' => array('fm_trans_max')), _fm_strip($form_state['values']['fm_trans_max']), $form_state);
	form_set_value(array('#parents' => array('fm_overdraw_limit')), _fm_parse_float($form_state['values']['fm_overdraw_limit']), $form_state);

}
