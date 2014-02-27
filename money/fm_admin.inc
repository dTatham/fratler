<?php
/**
 * @file
 * Admin-Interface and everything related
 */

/**
 * Page callback: fratler_money settings
 *
 * @see fratler_money_menu()
 */
function fratler_money_admin_form($form, &$form_state) {
	// Load toolchain
	if (!function_exists('_fm_cs'))
		module_load_include('inc', 'fratler_money', 'fm_tools');

	// Initialize formatter-object
	$fmt	= _fm_nf_init();

	$form['general'] 	= array(
		'#type'		=> 'fieldset',
		'#title'	=> t('General Settings'),
	);

	// We'll use the "intl" Framework PHP ships with, which allows for better flexibility and it doesn't give a flying f... about installed locales
	$form['general']['fm_locale'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Used locale'),
		'#default_value' => variable_get('fm_locale'),
		'#size'		=> 10,
		'#maxlength'	=> 30,
		'#description'	=> t('Please enter a locale-String you would like to use to format the amounts (e.g. en_US or de_DE).<br />Enter "C" as fallback.'),
		'#required'	=> true,
		'#field_suffix'	=> t('Currently using "@locale" (example: @num)', array('@locale' => variable_get('fm_locale', 'C'), '@num' => _fm_monetize(-1234567.890, $fmt))),
	);

	// Pager-Settings (how many entries per page)
	$form['general']['fm_pager'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Statement entries per page'),
		'#description'	=> t('How many statement records would you like to display per page?<br />Enter "0" to disable pagination.'),
		'#size'		=> 5,
		'#maxlength'	=> 3,
		'#default_value' => variable_get('fm_pager'),
	);

	// Moving on to the transfer-settings
	$form['transfer'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('Transaction related settings'),
	);

	$form['transfer']['fm_trans_admin'] = array(
		'#type'		=> 'checkbox',
		'#title'	=> t('Allow admin to participate in transactions'),
		'#description'	=> t('Check this, if you want the admin-user to participate in the transaction-system (i.e. receive transfers).'),
		'#default_value' => variable_get('fm_trans_admin'),
	);		

	// Maximum transaction-amount
	$form['transfer']['fm_trans_max'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Maximum transaction amount'),
		'#description'	=> t('The highest amount that can be transferred in one transaction.<br />Enter 0 for unlimited transactions.'),
		'#default_value' => _fm_numerize(variable_get('fm_trans_max')),
		'#size'		=> 8,
		'#field_prefix'	=> _fm_cs_psfix(true, $fmt),
		'#field_suffix' => _fm_cs_psfix(false, $fmt),
		'#attributes'   => array('class' => array('fm_amount')),
	);

	// Overdraw-Limit
        $form['transfer']['fm_overdraw_limit'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Overdraw limit'),
		'#description'	=> t('The amount a normal user may overdraw its amount by.<br />' .
					'Enter a positive value to keep a "safety-deposit".<br />' .
					'NOTE: If you enter a positive value, the permission @permission loses it\'s effect.', array('@permission' => 'Overdraw own account')),
		'#default_value' => _fm_numerize(variable_get('fm_overdraw_limit')),
		'#size'		=> 8,
		'#field_prefix'	=> _fm_cs_psfix(true, $fmt),
		'#field_suffix' => _fm_cs_psfix(false, $fmt),
		'#attributes'   => array('class' => array('fm_amount')),
	);


	return system_settings_form($form);
}

/**
 * implements form_validate()
 */
function fratler_money_admin_form_validate($form, &$form_state) {
	// Gimme dem tools!
	if (!function_exists('_fm_cs'))
		module_load_include('inc', 'fratler_money', 'fm_tools');


	// Work the usual magic...
	if (!_fm_nf_init(NumberFormatter::IGNORE, $form_state['values']['fm_locale']))
		form_set_error('fm_locale', t('Invalid locale.'));
	if (!is_numeric($form_state['values']['fm_pager']) || $form_state['values']['fm_pager'] < 0)
		form_set_error('fm_pager', t('Invalid entries per page setting'));
	if (!is_numeric(_fm_parse_number($form_state['values']['fm_trans_max'])) || _fm_parse_number($form_state['values']['fm_trans_max']) < 0)
		form_set_error('fm_trans_max', t('Invalid maximum transaction amount'));
       if (!is_numeric(_fm_parse_number($form_state['values']['fm_overdraw_limit'])))
		       form_set_error('fm_overdraw_limit', t('Invalid overdraw amount'));
	else	// Convert amount
		form_set_value($form['transfer']['fm_overdraw_limit'], _fm_parse_number($form_state['values']['fm_overdraw_limit']), $form_state);

	// Convert transaction-amount
	form_set_value($form['transfer']['fm_trans_max'], _fm_parse_number($form_state['values']['fm_trans_max']), $form_state);

	
}
