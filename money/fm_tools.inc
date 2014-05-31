<?php
/**
  * Strip the locale's thousands-separator and replace the locale's decimal separator with a '.'
  * Returns the float-value (if the passed argument is not a number or malformed there WILL BE funky business!)
  *
  * @param number
  *	The number
  */
function _fm_parse_float($number) {
	$fm_locale = localeconv();
	$number = str_replace($fm_locale['mon_thousands_sep'] , '', $number);
	$number = str_replace($fm_locale['mon_decimal_point'] , '.', $number);
	$number = str_replace($fm_locale['currency_symbol'], '', $number);
	$number = str_replace(' ', '', $number);

	return floatval($number);
}

/**
  * Strip the locale's thousands-separator and replace the locale's decimal separator with a '.'
  *
  * @param number
  *	The number
  */
function _fm_strip($number) {
	$fm_locale = localeconv();
	
	$number = str_replace($fm_locale['mon_thousands_sep'], '', $number);	
	$number = str_replace($fm_locale['mon_decimal_point'], '.', $number);	
	$number = str_replace($fm_locale['currency_symbol'], '', $number);
	$number = str_replace(' ', '', $number);

	return $number;
}

/**
 * returns the currency-symbol
 */
function _fm_cs() {
	$fm_locale = localeconv();
	return $fm_locale['currency_symbol'];
}
	
/**
 * Fetch the balance of a user
 *
 * @param user
 * 	User-ID
 * @param date
 * 	Get the balance of a certain date (the returned balance will be the latest one close to the passed date)
 * 	default = 0 (latest date)
 */
function _fm_balance($user, $date = 0) {
	if ($date == 0) 
		return floatval(db_query('SELECT balance FROM {fratler_money_balances} WHERE uid = :uid', array(':uid' => $user), array('fetch' => PDO::FETCH_NUM))->fetchField());
	else	
		return floatval(db_query('SELECT balance FROM {fratler_money} WHERE rid = :uid AND timestamp <= :date ORDER BY timestamp LIMIT 1', array(':uid' => $user, ':date' => $date)), array('fetch' => PDO::FETCH_NUM))->fetchField();
	return floatval(0);
}

/**
 * Strip unnecessary and potentially dangerous HTML-Tags
 *
 * @param text
 * 	the raw text
 */
function _fm_strip_html($text) {
	return filter_xss($text, array('em', 'strong', 'cite', 'blockquote', 'code', 'ul', 'ol', 'li', 'dl', 'dt', 'dd'));
}
/**
 * Calculate a transaction checksum
 *
 * @param sid
 * 	The sender-userID
 * @param rid
 * 	The beneficiary user-ID
 * @param iid
 * 	The initiating user's user-ID
 * @param amount
 * 	The transaction amount (if negative, sender / beneficiary will be swapped; the amount itself is used unsigned)
 * @param text
 * 	The transaction-Text
 * @param timestamp
 * 	The transaction-timestamp
*/
function _fm_hash($sid, $rid, $iid, $type, $amount, $text, $timestamp) {
	if ($amount > 0) 
		return md5('s' . $sid . 'c' . $rid . 'k' . $iid . 'm' . $type . 'y' .  $amount . 'd' . $text . 'c' . $timestamp . 'k?');

	else
		return md5('s' . $rid . 'c' . $sid . 'k' . $iid . 'm' . $type . 'y' . abs($amount) . 'd' . $text . 'c' . $timestamp . 'k?');
}	

/**
 * Return the uniform hashing-string
 *
 * @param sid
 * 	The sender-userID
 * @param rid
 * 	The beneficiary user-ID
 * @param iid
 * 	The initiating user's user-ID
 * @param amount
 * 	The transaction amount (if negative, sender / beneficiary will be swapped; the amount itself is used unsigned)
 * @param text
 * 	The transaction-Text
 * @param timestamp
 * 	The transaction-timestamp
*/
function _fm_hash_prepare($sid, $rid, $iid, $type, $amount, $text, $balance, $timestamp) {
		return 's' . $sid . 'c' . $rid . 'k' . $iid . 'm' . $type . 'y' . number_format($amount, 2, '.', '') . 'd' . $text . 'c' . number_format($balance, 2, '.', '') . 'k' . $timestamp . '?';
}

/**
 * Create a transaction signature
 *
 * @param sid
 * 	The sender-userID
 * @param rid
 * 	The beneficiary user-ID
 * @param iid
 * 	The initiating user's user-ID
 * @param amount
 * 	The transaction amount (if negative, sender / beneficiary will be swapped; the amount itself is used unsigned)
 * @param text
 * 	The transaction-Text
 * @param balance
 * 	The new balance (since it's an individual signature...)
 * @param timestamp
 * 	The transaction-timestamp
 */
function _fm_sign($sid, $rid, $iid, $type, $amount, $text, $balance, $timestamp) {
	openssl_sign(_fm_hash_prepare($sid, $rid, $iid, $type, $amount, $text, $balance, $timestamp), $signature, variable_get('fc_key'));
	return $signature;
}

/**
 * Check, if user is allowed to participate in the transaction system
 *
 * @param user
 * 	a user-object
 */
function _fm_check_participant(&$user) {
	foreach (array_keys($user->roles) as $role) 
		if (in_array($role, array_values(variable_get('fm_transfer_groups'))))
			return true;
	return false;
}
