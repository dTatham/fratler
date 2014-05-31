<?php
/**
 * @file
 * Admin-Section
 */

/**
 * Page callback: fratler_money settings
 *
 * @see fratler_common_menu()
 */
function fratler_common_admin_form($form, &$form_state) {
	$no_key = " --- NO KEY GENERATED ---\nThis is bad. Very very bad!!!";
	$form['fc_key'] = array(
		'#type'		=> 'textarea',
		'#title'	=> t('Private Key'),
		'#default_value' => variable_get('fc_key', $no_key),
		'#description'	=> t('This is the PEM-formatted private key used to sign off certain actions made by the fratler_suite.') . '<br /><b>' . t('As the name says: it\'s a PRIVATE key - do not hand it out to anyone!') . '<br />' .  t('DO NOT EDIT THIS UNLESS YOU KNOW WHAT YOU ARE DOING!!!') . '</b>',
		'#required'	=> true,
		'#rows'		=> 16,
	);
	$form['fc_key_public'] = array(
		'#type'		=> 'textarea',
		'#title'	=> t('Public Key'),
		'#default_value' => variable_get('fc_key_public', $no_key),
		'#description'	=> t('This is the key used to sign off certain actions made by the fratler_suite.') . '<br /><b>' . t('AGAIN: DO NOT EDIT THIS UNLESS YOU KNOW WHAT YOU ARE DOING!!!') . '</b>',
		'#rows'		=> 7,
	);
	$form[] = array(
		'#type'		=> 'checkbox',
		'#title'	=> t('Regenerate Keys'),
		'#description'	=> t('If checked, a new key-pair is created after you hit ":save".<br />NOTICE: You need to re-sign everything after you generated new keys.<br />You know what\'s coming next, right?', array(':save' => 'Save configuration')) . '<br /><b>' . t('HANDS OFF UNLESS YOU KNOW WHAT YOU ARE DOING!!!') . '</b>',
		'#default_value' => false,
	);

	return system_settings_form($form);
}

function fratler_common_admin_form_validate($form, &$form_state) {

	/* Really? */
	if ($form_state['values'][0] == "1") {
		/* Yep. They're crazy. Alright - regenerating keys... */

		// Generate a new key-pair and store it
		// TODO: Error-Handling!
		// from: fratler_common.install
		$res = openssl_pkey_new();
		$key = false;
		openssl_pkey_export($res, $key);
		form_set_value(array('#parents' => array('fc_key')), $key, $form_state);

		$pubkey = openssl_pkey_get_details($res);
		form_set_value(array('#parents' => array('fc_key_public')), $pubkey['key'], $form_state);
	}

	/* If the smart-ass supplied his own keys, check if they match */
	if ($form_state['values']['fc_key'] != variable_get('fc_key')) {
		$res = openssl_pkey_get_private($form_state['values']['fc_key']);
		if (!$res) {
			form_set_error('fc_key', t('Invalid private key. Told you not to mess with it...'));
			return;
		}

		$pubkey = openssl_pkey_get_details($res);
		if ($form_state['values']['fc_key_public'] != $pubkey['key'])  {
			form_set_value(array('#parents' => array('fc_key_public')), $pubkey['key'], $form_state);
			drupal_set_message(t('Your keys did not match. I took the liberty and replaced the public key with the correct one.'), 'warning');
		}

	}
	if (empty($form_state['values']['fc_key_public']) || $form_state['values']['fc_key_public'] != variable_get('fc_key_public')) {
		$res = openssl_pkey_get_private($form_state['values']['fc_key']);
		if (!$res) {
			form_set_error('fc_key', t('Invalid private key. Told you not to mess with it...'));
			return;
		}

		$pubkey = openssl_pkey_get_details($res);
		form_set_value(array('#parents' => array('fc_key_public')), $pubkey['key'], $form_state);
		drupal_set_message(t('I took the liberty and (re-)created the public key.'));
	}

}
