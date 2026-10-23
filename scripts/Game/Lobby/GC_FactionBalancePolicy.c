//! Proportional quotas with deterministic largest-remainder rounding.
//! Callers supply faction capacities in stable faction-key order.
class GC_FactionBalancePolicy
{
	//------------------------------------------------------------------------------------------------
	static void Calculate(array<int> capacities, int players, out array<int> quotas)
	{
		quotas = {};
		array<float> remainders = {};
		int total;
		foreach (int capacity : capacities)
			total += Math.Max(0, capacity);
		int assigned;
		foreach (int capacity : capacities)
		{
			float exact;
			if (total > 0 && players > 0 && capacity > 0)
				exact = players * (capacity / (float)total);
			int quota = Math.Floor(exact);
			quotas.Insert(quota);
			remainders.Insert(exact - quota);
			assigned += quota;
		}
		if (total == 0)
			return;
		while (assigned < players)
		{
			int best = -1;
			float largest = -1;
			foreach (int index, float remainder : remainders)
			{
				if (capacities[index] > 0 && remainder > largest)
				{
					best = index;
					largest = remainder;
				}
			}
			if (best < 0)
				return;
			quotas[best] = quotas[best] + 1;
			remainders[best] = -1;
			assigned++;
		}
	}
}
