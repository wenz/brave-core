/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext } from '../lib/host_context'
import { LocaleContext } from '../../shared/lib/locale_context'
//import { formatExchangeAmount, formatLocaleTemplate } from '../lib/formatting'

import { ExchangeIcon } from './icons/exchange_icon'

import * as style from './custom_amount_input.style'

interface Props {
  amount: number
  onAmountChange: (amount: number) => void
}

export function CustomAmountInput (props: Props) {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  const [rewardsParameters, setRewardsParameters] = React.useState(
    host.state.rewardsParameters)

  React.useEffect(() => {
    return host.addListener((state) => {
      setRewardsParameters(state.rewardsParameters)
    })
  }, [host])

  return (
    <style.root>
      <ExchangeIcon />
      {rewardsParameters && rewardsParameters.rate}
      {getString('asdf')}
    </style.root>
  )
}
