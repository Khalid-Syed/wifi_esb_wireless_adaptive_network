

static int get_nearest(int x, int y, int target);
/**
 * @brief Get the nearest element index object
 *
 * @param arr
 * @param length
 * @param target
 * @return int
 */
int get_nearest_element_index(const int *arr, int length, int target)
{
    if (target <= arr[0])
    {
        return 0;
    }
    if (target >= arr[length - 1])
    {
        return length - 1;
    }

    int left = 0, right = length, mid = 0;
    while (left < right)
    {
        mid = (left + right) / 2;
        if (arr[mid] == target)
        {
            return mid;
        }
        if (target < arr[mid])
        {
            if (mid > 0 && target > arr[mid - 1])
            {
                int nearest = get_nearest(arr[mid - 1], arr[mid], target);
                if (nearest == arr[mid - 1])
                {
                    return mid - 1;
                }
                else
                {
                    return mid;
                }
                return mid;
            }
            right = mid;
        }
        else
        {
            if (mid < length - 1 && target < arr[mid + 1])
            {
                int nearest = get_nearest(arr[mid], arr[mid + 1], target);
                if (nearest == arr[mid])
                {
                    return mid;
                }
                else
                {
                    return mid + 1;
                }
            }
            left = mid + 1;
        }
    }
    return mid;
}
int get_nearest(int x, int y, int target)
{
    if (target - x >= y - target)
    {
        return y;
    }
    return x;
}
