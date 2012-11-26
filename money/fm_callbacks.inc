<?php
/**
 * @file
 * Since AJAX-Callbacks aren't very pretty, they are dumped here.
 */

/**
 * user-autocomplete
 *
 * @param string
 * 	the partial string passed by #autocomplete_path
 */
function _fm_autocomplete($string = '') {
	$query= db_select('users', 'u');
	$query->fields('u', array('name'));

	if (!in_array(2, variable_get('fm_transfer_groups')) && !user_access('fm transfer other')) { 		// If the user-role "authenticated user" is set as enabled to participate in the transactions, there's no need to cross-reference it with the roles-table...
		$query->join('users_roles', 'r', 'u.uid = r.uid');
		$db_or = db_or();
		foreach (variable_get('fm_transfer_groups') as $role) {
			if ($role)
				$db_or->condition('r.rid', $role);
		}
		$query->condition($db_or);
	}

	$query->condition('u.name', db_like($string) . '%', 'LIKE')->range(0, 10);
	$matches = array();
	foreach ($query->execute() as $result)
		$matches[$result->name] = $result->name;

	drupal_json_output($matches);
	
}

/**
 * Display the account-statement data
 *
 * @param uid
 * 	the user-object of the desired account
 */
function _fm_user_display_callback($uid) {
	header("Content-type: text/html");
	header("Expires: Wed, 29 Jan 1975 04:15:00 GMT");
	header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
	header("Cache-Control: no-cache, must-revalidate");
	header("Pragma: no-cache");

	$header = array(
		array('data' => t('id',array(),array('context' => 'Transaction-ID')), 'field' => 'tid', 'sort' => 'desc', 'class' => array('fm_header')),
		array('data' => t('Timestamp'), 'field' => 'timestamp', 'class' => array('fm_header')),
		array('data' => t('Type'), 'class' => array('fm_header')),
		array('data' => t('Message'), 'class' => array('fm_message')),
		array('data' => t('Amount'), 'class' => array('fm_header')),
		array('data' => t('New Balance'), 'class' => array('fm_header')),
		array('data' => t('Fingerprint'), 'class' => array('fm_header')),
		array('data' => t('Signature'), 'class' => array('fm_header')),
	);

	$query = db_select('fratler_money', 'f', array('fetch' => 'fm_transaction_detail'))
		->extend('PagerDefault')
		->limit(variable_get('fm_pager', 10))
		->extend('TableSort')
		->orderByHeader($header)
		->fields('f', array('tid', 'timestamp', 'rid', 'sid', 'iid', 'type', 'text', 'amount', 'balance', 'fingerprint', 'signature'))
		->condition('rid', $uid->uid)
		->execute();


	$rows = array();

	foreach ($query as $data) {
		$rows[$data->tid] = array(
			$data->tid,
			array('data' => format_date($data->timestamp, 'short'), 'class' => array('fm_header')),
			array('data' => taxonomy_term_load($data->type)->name),
			($data->amount < 0 ? t('To') .': ' . format_username($data->sid_data) . '<br />' : '') . 
				($data->amount > 0 ? t('From') . ': ' . format_username($data->sid_data) . '<br />' : '') . 
				($data->iid != 0 ? t('By') . ': ' . format_username($data->iid_data) . '<br />' : '')  . $data->text,
			array('data' => money_format("%+n", $data->amount), 'class' => array('fm_amount', $data->amount > 0 ? 'fm_credit' : 'fm_debit')),
			array('data' => money_format("%n", $data->balance), 'class' => array('fm_amount', $data->balance > 0 ? 'fm_credit' : 'fm_debit')),
			l(hash('crc32b', $data->fingerprint), 'fratler/money/display/' . $data->fingerprint),
			$data->check_signature() ? 'OK' : 'Error',
		);
	}

	$html = theme('table',  array(
		'header'	=> $header,
		'rows'		=> $rows,
		'sticky'	=> true,
		)
	);
	$html .= theme('pager', array('tags' => array()));
	die($html);

}

